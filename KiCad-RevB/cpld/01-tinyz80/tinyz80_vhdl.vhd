----------------------------------------------------------------------------------S
-- Company: 
-- Engineer: 
-- 
-- Create Date:    11:53:56 07/30/2023 
-- Design Name: 
-- Module Name:    tinyz80 - Behavioral 
-- Project Name: 
-- Target Devices: 
-- Tool versions: 
-- Description: 
--
-- Dependencies: 
--
-- Revision: 
-- Revision 0.01 - File Created
-- Additional Comments: 
--
----------------------------------------------------------------------------------
library IEEE;
use IEEE.STD_LOGIC_1164.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx primitives in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity tinyz80 is
    port
    (
        A : in std_logic_vector(7 downto 0);
        A_HI : in std_logic_vector(15 downto 14);
        D : inout std_logic_vector(7 downto 0);
        M1 : in std_logic;
        IORQ : in std_logic;
        MREQ : in std_logic;
        RD : in std_logic;
        WR : in std_logic;
        WDOG_WR : out std_logic;
        LED_OUT : out std_logic;
        EX1_OUT : out std_logic;
        RSTn : in std_logic;
        ROM_CSn : out std_logic;
        RAM_CSn : out std_logic;
        RNG_CSn : out std_logic;
        I2C_CSn : out std_logic;
        SPI_CSn : out std_logic;
        CSR_CSn : out std_logic;
        CSN_CSn : out std_logic;
        CSS_CSn : out std_logic;
        MODE : out std_logic;
        GROM_CLK : out std_logic;

        UART_CLK : in std_logic;
        CPU_CLK : in std_logic;

        SD_DETECT : in std_logic;
        
        CPU_CLK_3V : buffer std_logic;
        CPU_CLK_3V_B : buffer std_logic;
        CTC_CLK : buffer std_logic;

        MA : out std_logic_vector(18 downto 14)
    );
    -- Page Registers addresses are 0x78-0x7B = 011110xx
    -- TODO four addresses, four registers necesary
    constant PAGE_ADDR : std_logic_vector(7 downto 2) := "011110";   -- 78..7b
    constant PGEN_ADDR : std_logic_vector(7 downto 0) := "01111100"; -- 7c
    -- Watchdog address is 0x6F = 01101111
    constant WDOG_ADDR : std_logic_vector(7 downto 0) := "01101111"; -- 6f
    -- LED address is 0x6E = 01101110
    constant LED_ADDR : std_logic_vector(7 downto 0) := "01101110"; -- 6e
    constant RNG_ADDR : std_logic_vector(7 downto 0) := "11111001"; -- f9
    constant CSN_ADDR : std_logic_vector(7 downto 0) := "11111010"; -- fa
    constant CSR_ADDR : std_logic_vector(7 downto 0) := "11111011"; -- fb
    constant CSS_ADDR : std_logic_vector(7 downto 0) := "11111100"; -- fc
    constant I2C_ADDR : std_logic_vector(7 downto 0) := "11111101"; -- fd
    constant SPI_ADDR : std_logic_vector(7 downto 0) := "11111110"; -- fe
    constant SD_DETECT_ADDR : std_logic_vector(7 downto 0) := "11101010"; -- ea
end tinyz80;

architecture Behavioral of tinyz80 is
    signal IOWR, IORD, LED_WR, PGEN_WR, PAGE_ENA, PAGE_WR, PAGE_RD, CTC_CLK_INTERNAL, CPU_CLK_INTERNAL : std_logic;
    signal PA : std_logic_vector(19 downto 14);
    signal rdaddress : std_logic_vector(1 downto 0);

    component D_FF_RST port (CLK, RSTn, D : in std_logic; Q : out std_logic); end component;
    component page_reg_4x6_lpm PORT
    (
        CLK : in std_logic;
        RSTn : in std_logic;
        data : IN STD_LOGIC_VECTOR (5 DOWNTO 0);
        -- op : in std_logic;
        rdaddress : IN STD_LOGIC_VECTOR (1 DOWNTO 0);
        wraddress : IN STD_LOGIC_VECTOR (1 DOWNTO 0);
        wren : IN STD_LOGIC  := '0';
        q : OUT STD_LOGIC_VECTOR (5 DOWNTO 0)
    );
    end component;
    
    component CLOCK_DIVIDER PORT
    (
        CLK : IN std_logic;
        RSTn : IN std_logic;
        Q : OUT std_logic
    );
    end component;

begin
    -- CPU control signals and address decode
    IOWR <= '1' when M1 = '1' and IORQ = '0' and WR = '0' else '0';
    IORD <= '1' when M1 = '1' and IORQ = '0' and RD = '0' else '0';
    PAGE_WR <= '1' when IOWR = '1' and A(7 downto 2) = PAGE_ADDR else '0';
    PAGE_RD <= '1' when IORD = '1' and A(7 downto 2) = PAGE_ADDR else '0';
    PGEN_WR <= '1' when IOWR = '1' and A = PGEN_ADDR else '0';
    -- WDOG_WR <= '1' when IOWR = '1' and A = WDOG_ADDR else '0';
    WDOG_WR <= M1;
    LED_WR <= '1' when IOWR = '1' and A = LED_ADDR else '0';

    -- CTC clock divisor
    CTC_CLK_FF : D_FF_RST port map(CLK => UART_CLK, RSTn => RSTn, D => not CTC_CLK, Q => CTC_CLK_INTERNAL);
    
    CTC_CLK <= CTC_CLK_INTERNAL;

    -- LED FF, EX0
    LED_FF : D_FF_RST port map(CLK => LED_WR, RSTn => RSTn, D => D(0), Q => LED_OUT);
    -- EX1
    -- EX1_FF : CLOCK_DIVIDER port map(CLK => CPU_CLK, RSTn => RSTn, Q => EX1_OUT);

    -- Paging enable FF
    PAGE_ENA_FF : D_FF_RST port map(CLK => PGEN_WR, RSTn => RSTn, D => D(0), Q => PAGE_ENA);
    -- debugging
    EX1_OUT <= PAGE_ENA;
    
    rdaddress <= A(1 downto 0) when PAGE_RD = '1' else A_HI;

    -- Page register
    PAGE_REG : page_reg_4x6_lpm port map(CLK => CPU_CLK, RSTn => RSTn,
        data => D(5 downto 0),
        -- op => RD,
        -- rdaddress => A_HI,
        -- rdaddress => b"00",
        rdaddress => rdaddress,
        wraddress => A(1 downto 0),
        wren => PAGE_WR,
        q => PA);

    -- MA <= b"00000";
    MA <= PA(18 downto 14) when PAGE_ENA = '1' else b"00000";

    -- Memory chip select
    -- ROM_CSn <= '0' when A_HI(15) = '0' and MREQ = '0' else '1';
    -- RAM_CSn <= '0' when A_HI(15) = '1' and MREQ = '0' else '1';

    ROM_CSn <= '0' when ((PA(19) = '0' and PAGE_ENA = '1') or (A_HI(15) = '0' and PAGE_ENA = '0')) and MREQ = '0' else '1';
    RAM_CSn <= '0' when ((PA(19) = '1' and PAGE_ENA = '1') or (A_HI(15) = '1' and PAGE_ENA = '0')) and MREQ = '0' else '1';

    -- ROM_CSn <= '0' when (PA(19) = '0' or PAGE_ENA = '0') and MREQ = '0' else '1';
    -- ROM_CSn <= '0' when ((PA(19) = '0' and PAGE_ENA = '0') or (A_HI(15) = '0' and PAGE_ENA = '0')) and MREQ = '0' else '1';
    -- ROM_CSn <= '0' when (PA(19) = '0' or PAGE_ENA = '0') and MREQ = '0' and WR = '1' else '1';

    -- RAM_CSn <= '0' when (PA(19) = '1' and PAGE_ENA = '1') and MREQ = '0' else '1';
    -- RAM_CSn <= '0' when ((PA(19) = '1' and PAGE_ENA = '1') or (A_HI(15) = '1' and PAGE_ENA = '0')) and MREQ = '0' else '1';
    -- RAM_CSn <= '0' when ((PA(19) = '1' and PAGE_ENA = '1') or (PA(19) = '0' and PAGE_ENA = '1' and WR = '1')) and MREQ = '0' else '1';

    RNG_CSn <= '0' when IORD = '1' and A(7 downto 0) = RNG_ADDR else '1';

    -- TODO should be 1 not Z, CPLD is the sole source for these signals
    I2C_CSn <= '0' when ((IOWR = '1' or IORD = '1') and A(7 downto 0) = I2C_ADDR) else '1';
    SPI_CSn <= '0' when ((IOWR = '1' or IORD = '1') and A(7 downto 0) = SPI_ADDR) else '1';

    CSR_CSn <= '0' when ((IOWR = '1' or IORD = '1') and A(7 downto 0) = CSR_ADDR) else '1';
    CSN_CSn <= '0' when ((IOWR = '1' or IORD = '1') and A(7 downto 0) = CSN_ADDR) else '1';
    -- TODO multiple addresses to support MODE
    MODE <= A(0) when (IOWR = '1' and A(7 downto 0) = CSR_ADDR) else 'Z';
    -- TODO this is not used and should be converted to IEO
    GROM_CLK <= CPU_CLK;

    CSS_CSn <= '0' when (IOWR = '1' or IORD = '1') and A(7 downto 0) = CSS_ADDR else 'Z';

    -- signals to tang nano 9k, cpld used as level shifter
    -- CPU_CLK_FF : D_FF_RST port map(CLK => CPU_CLK, RSTn => RSTn, D => CPU_CLK, Q => CPU_CLK_INTERNAL);
    CPU_CLK_3V <= CPU_CLK; -- CPU_CLK_INTERNAL;
    CPU_CLK_3V_B <= CPU_CLK;
    -- IORQ_3V <= '0' when IORQ = '0' else '1';

    -- data bus
    D(7) <= SD_DETECT when (IORD = '1' and A(7 downto 0) = SD_DETECT_ADDR) else 'Z';
    D(6) <= PAGE_ENA when PAGE_RD = '1' else 'Z';
    D(5 downto 0) <= PA(19 downto 14) when PAGE_RD = '1' else (others => 'Z');

end Behavioral;
