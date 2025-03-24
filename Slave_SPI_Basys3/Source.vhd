library IEEE;
use IEEE.STD_LOGIC_1164.ALL;
use IEEE.NUMERIC_STD.ALL;

entity Source is
    Port (
        clk   : in  std_logic;     -- Horloge 100 MHz
        SCK   : in  std_logic;     -- SPI clock
        CS    : in  std_logic;     -- Chip Select (actif bas)
        MOSI  : in  std_logic;     -- Donnée maître → esclave
        MISO  : out std_logic;     -- Donnée esclave → maître
        led0  : out std_logic      -- LED de contrôle
    );
end Source;

architecture Behavioral of Source is

    type state_type is (IDLE, RECEIVE, DO_CMD, SEND);
    signal state, next_state : state_type := IDLE;

    -- SPI synchronisation
    signal sck_sync    : std_logic_vector(1 downto 0) := (others => '0');
    signal cs_sync     : std_logic_vector(1 downto 0) := (others => '0');
    signal mosi_sync   : std_logic_vector(1 downto 0) := (others => '0');

    signal sck_rising  : std_logic := '0';
    signal cs_active   : std_logic := '0';

    -- SPI logiques
    signal bit_count   : integer range 0 to 8 := 0;
    signal byte_in     : std_logic_vector(7 downto 0) := (others => '0');
    signal byte_out    : std_logic_vector(7 downto 0) := (others => '0');
    signal miso_reg    : std_logic := '0';

begin

    -- Sortie MISO
    MISO <= miso_reg;

    -- 1. État suivant
    process(state, cs_active, bit_count)
    begin
        case state is
            when IDLE =>
                if cs_active = '1' then
                    next_state <= RECEIVE;
                else
                    next_state <= IDLE;
                end if;

            when RECEIVE =>
                if bit_count = 8 then
                    next_state <= DO_CMD;
                else
                    next_state <= RECEIVE;
                end if;

            when DO_CMD =>
                next_state <= SEND;

            when SEND =>
                if bit_count = 8 then
                    next_state <= IDLE;
                else
                    next_state <= SEND;
                end if;

            when others =>
                next_state <= IDLE;
        end case;
    end process;

    -- 2. État courant
    process(clk)
    begin
        if rising_edge(clk) then
            state <= next_state;
        end if;
    end process;

    -- 3. Logique SPI + actions
    process(clk)
    begin
        if rising_edge(clk) then
            -- Synchronisation des signaux
            sck_sync  <= sck_sync(0) & SCK;
            cs_sync   <= cs_sync(0) & CS;
            mosi_sync <= mosi_sync(0) & MOSI;
if sck_sync = "01" then
    sck_rising <= '1';
else
    sck_rising <= '0';
end if;

            cs_active  <= not cs_sync(1);

            case state is
                when IDLE =>
                    bit_count <= 0;
                    miso_reg <= '0';

                when RECEIVE =>
                    if sck_rising = '1' then
                        byte_in <= byte_in(6 downto 0) & mosi_sync(1);
                        bit_count <= bit_count + 1;
                    end if;

                when DO_CMD =>
                    case byte_in is
                        when x"01" =>
                            led0 <= '1';
                            byte_out <= x"AA";
                        when x"02" =>
                            led0 <= '0';
                            byte_out <= x"BB";
                        when others =>
                            byte_out <= x"EE";
                    end case;
                    bit_count <= 0;
                    miso_reg <= byte_out(7);

                when SEND =>
                    if sck_rising = '1' then
                        bit_count <= bit_count + 1;
                        if bit_count < 8 then
                            miso_reg <= byte_out(7 - bit_count);
                        else
                            miso_reg <= '0';
                        end if;
                    end if;
            end case;
        end if;
    end process;

end Behavioral;