--------------------------------------------------------------------------------
-- Company: 
-- Engineer:
--
-- Create Date:   13:13:21 09/24/2023
-- Design Name:   
-- Module Name:   /home/anru/Projects/bitbucket/schematics/z80/14-z80-flash-tang-nano-9k/cpld/01-tinyz80/test_xpu_clk.vhd
-- Project Name:  tinyz80
-- Target Device:  
-- Tool versions:  
-- Description:   
-- 
-- VHDL Test Bench Created by ISE for module: tinyz80
-- 
-- Dependencies:
-- 
-- Revision:
-- Revision 0.01 - File Created
-- Additional Comments:
--
-- Notes: 
-- This testbench has been automatically generated using types std_logic and
-- std_logic_vector for the ports of the unit under test.  Xilinx recommends
-- that these types always be used for the top-level I/O of a design in order
-- to guarantee that the testbench will bind correctly to the post-implementation 
-- simulation model.
--------------------------------------------------------------------------------
LIBRARY ieee;
USE ieee.std_logic_1164.ALL;
 
-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--USE ieee.numeric_std.ALL;
 
ENTITY test_xpu_clk IS
END test_xpu_clk;
 
ARCHITECTURE behavior OF test_xpu_clk IS 
 
    -- Component Declaration for the Unit Under Test (UUT)
 
    COMPONENT tinyz80
    PORT(
         A : IN  std_logic_vector(7 downto 0);
         A_HI : IN  std_logic_vector(15 downto 14);
         D : INOUT  std_logic_vector(7 downto 0);
         M1 : IN  std_logic;
         IORQ : IN  std_logic;
         MREQ : IN  std_logic;
         RD : IN  std_logic;
         WR : IN  std_logic;
         WDOG_WR : OUT  std_logic;
         LED_OUT : OUT  std_logic;
         EX1_OUT : OUT  std_logic;
         RSTn : IN  std_logic;
         ROM_CSn : OUT  std_logic;
         RAM_CSn : OUT  std_logic;
         RNG_CSn : OUT  std_logic;
         I2C_CSn : OUT  std_logic;
         SPI_CSn : OUT  std_logic;
         CSR_CSn : OUT  std_logic;
         CSN_CSn : OUT  std_logic;
         CSS_CSn : OUT  std_logic;
         MODE : OUT  std_logic;
         GROM_CLK : OUT  std_logic;
         UART_CLK : IN  std_logic;
         CPU_CLK : IN  std_logic;
         SD_DETECT : IN  std_logic;
         CPU_CLK_3V : buffer std_logic;
         CPU_CLK_3V_B : buffer std_logic;
         CTC_CLK : buffer std_logic;
         MA : OUT  std_logic_vector(18 downto 14)
        );
    END COMPONENT;
    

   --Inputs
   signal A : std_logic_vector(7 downto 0) := (others => '0');
   signal A_HI : std_logic_vector(15 downto 14) := (others => '0');
   signal M1 : std_logic := '0';
   signal IORQ : std_logic := '0';
   signal MREQ : std_logic := '0';
   signal RD : std_logic := '0';
   signal WR : std_logic := '0';
   signal RSTn : std_logic := '0';
   signal UART_CLK : std_logic := '0';
   signal CPU_CLK : std_logic := '0';
   signal SD_DETECT : std_logic := '0';

	--BiDirs
   signal D : std_logic_vector(7 downto 0);

 	--Outputs
   signal WDOG_WR : std_logic;
   signal LED_OUT : std_logic;
   signal EX1_OUT : std_logic;
   signal ROM_CSn : std_logic;
   signal RAM_CSn : std_logic;
   signal RNG_CSn : std_logic;
   signal I2C_CSn : std_logic;
   signal SPI_CSn : std_logic;
   signal CSR_CSn : std_logic;
   signal CSN_CSn : std_logic;
   signal CSS_CSn : std_logic;
   signal MODE : std_logic;
   signal GROM_CLK : std_logic;
   signal CPU_CLK_3V : std_logic;
   signal CPU_CLK_3V_B : std_logic;
   signal CTC_CLK : std_logic;
   signal MA : std_logic_vector(18 downto 14);

   -- Clock period definitions
   constant GROM_CLK_period : time := 1000 ns;
   constant UART_CLK_period : time := 1000 ns;
   constant CPU_CLK_period : time := 1000 ns;
   constant CTC_CLK_period : time := 1000 ns;
   
BEGIN
 
	-- Instantiate the Unit Under Test (UUT)
   uut: tinyz80 PORT MAP (
          A => A,
          A_HI => A_HI,
          D => D,
          M1 => M1,
          IORQ => IORQ,
          MREQ => MREQ,
          RD => RD,
          WR => WR,
          WDOG_WR => WDOG_WR,
          LED_OUT => LED_OUT,
          EX1_OUT => EX1_OUT,
          RSTn => RSTn,
          ROM_CSn => ROM_CSn,
          RAM_CSn => RAM_CSn,
          RNG_CSn => RNG_CSn,
          I2C_CSn => I2C_CSn,
          SPI_CSn => SPI_CSn,
          CSR_CSn => CSR_CSn,
          CSN_CSn => CSN_CSn,
          CSS_CSn => CSS_CSn,
          MODE => MODE,
          GROM_CLK => GROM_CLK,
          UART_CLK => UART_CLK,
          CPU_CLK => CPU_CLK,
          SD_DETECT => SD_DETECT,
          CPU_CLK_3V => CPU_CLK_3V,
          CPU_CLK_3V_B => CPU_CLK_3V_B,
          CTC_CLK => CTC_CLK,
          MA => MA
        );

   -- Clock process definitions
   GROM_CLK_process :process
   begin
		GROM_CLK <= '0';
		wait for GROM_CLK_period/2;
		GROM_CLK <= '1';
		wait for GROM_CLK_period/2;
   end process;
 
   UART_CLK_process :process
   begin
		UART_CLK <= '0';
		wait for UART_CLK_period/2;
		UART_CLK <= '1';
		wait for UART_CLK_period/2;
   end process;
 
   CPU_CLK_process :process
   begin
		CPU_CLK <= '0';
		wait for CPU_CLK_period/2;
		CPU_CLK <= '1';
		wait for CPU_CLK_period/2;
   end process;
 
   CTC_CLK_process :process
   begin
		CTC_CLK <= '0';
		wait for CTC_CLK_period/2;
		CTC_CLK <= '1';
		wait for CTC_CLK_period/2;
   end process;
 

   -- Stimulus process
   stim_proc: process
      variable err_cnt: integer := 0;
   begin		
      -- hold reset state for 100 ns.
      wait for 100 ns;	

      RSTn <= '0';

      -- case 1
      wait for CPU_CLK_period*4;

      A <= "00000000";
      D <= "00000000";
      A_HI <= "00";
      M1 <= '1';
      IORQ <= '1';
      MREQ <= '0';
      RD <= '1';
      WR <= '1';

      wait for CPU_CLK_period*2;
      assert (MA="00000") report "Test1 Failed!" severity error;
      assert (ROM_CSn='0') report "Test1 ROM_CS Failed!" severity error;
      assert (RAM_CSn='1') report "Test1 RAM_CS Failed!" severity error;

      RSTn <= '1';

      -- case 2
      wait for CPU_CLK_period*4;
      A <= "00000000";
      D <= "00000000";
      A_HI <= "10";
      M1 <= '1';
      IORQ <= '1';
      MREQ <= '0';
      RD <= '1';
      WR <= '1';

      wait for CPU_CLK_period*2;
      assert (MA="00000") report "Test2 Failed!" severity error;
      assert (ROM_CSn='1') report "Test2 ROM_CS Failed!" severity error;
      assert (RAM_CSn='0') report "Test2 RAM_CS Failed!" severity error;

      -- case 3
      -- enable paging
      wait for CPU_CLK_period*4;
      A <= "01111100";
      D <= "00000001";
      wait for CPU_CLK_period*2;
      A_HI <= "00";
      M1 <= '1';
      IORQ <= '0';
      MREQ <= '1';
      RD <= '1';
      wait for CPU_CLK_period*1;
      WR <= '0';

      wait for CPU_CLK_period*2;
      IORQ <= '1';
      wait for CPU_CLK_period*1;
      WR <= '1';
      assert (MA="00000") report "Test3 Failed!" severity error;
      assert (ROM_CSn='1') report "Test2 ROM_CS Failed!" severity error;
      assert (RAM_CSn='1') report "Test2 RAM_CS Failed!" severity error;


      -- case 4
      wait for CPU_CLK_period*4;

      A <= "01111000";
      D <= "00000011";
      A_HI <= "00";
      M1 <= '1';
      IORQ <= '0';
      MREQ <= '1';
      RD <= '1';
      WR <= '0';

      wait for CPU_CLK_period*1;
      IORQ <= '1';
      wait for CPU_CLK_period*1;
      WR <= '1';
      assert (MA="00011") report "Test4 Failed!" severity error;


      -- case 5
      wait for CPU_CLK_period*4;

      A <= "01111001";
      D <= "00001010";
      A_HI <= "00";
      M1 <= '1';
      IORQ <= '0';
      MREQ <= '1';
      RD <= '1';
      WR <= '0';

      wait for CPU_CLK_period*2;
      assert (MA="00011") report "Test5 Failed!" severity error;


      -- case 6
      wait for CPU_CLK_period*4;

      A <= "01111001";
      D <= "00000000";
      A_HI <= "01";
      M1 <= '1';
      IORQ <= '1';
      MREQ <= '1';
      RD <= '1';
      WR <= '1';

      wait for CPU_CLK_period*2;
      assert (MA="01010") report "Test6 Failed!" severity error;

      -- insert stimulus here 

      wait;
   end process;

END;
