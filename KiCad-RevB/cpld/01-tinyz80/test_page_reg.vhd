--------------------------------------------------------------------------------
-- Company: 
-- Engineer:
--
-- Create Date:   13:18:43 09/24/2023
-- Design Name:   
-- Module Name:   /home/anru/Projects/bitbucket/schematics/z80/14-z80-flash-tang-nano-9k/cpld/01-tinyz80/test_page_reg.vhd
-- Project Name:  tinyz80
-- Target Device:  
-- Tool versions:  
-- Description:   
-- 
-- VHDL Test Bench Created by ISE for module: page_reg_4x6_lpm
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
 
ENTITY test_page_reg IS
END test_page_reg;
 
ARCHITECTURE behavior OF test_page_reg IS 
 
    -- Component Declaration for the Unit Under Test (UUT)
 
    COMPONENT page_reg_4x6_lpm
    PORT(
         CLK : IN  std_logic;
         RSTn : IN  std_logic;
         data : IN  std_logic_vector(5 downto 0);
         rdaddress : IN  std_logic_vector(1 downto 0);
         wraddress : IN  std_logic_vector(1 downto 0);
         wren : IN  std_logic;
         q : OUT  std_logic_vector(5 downto 0)
        );
    END COMPONENT;
    

   --Inputs
   signal CLK : std_logic := '0';
   signal RSTn : std_logic := '0';
   signal data : std_logic_vector(5 downto 0) := (others => '0');
   signal rdaddress : std_logic_vector(1 downto 0) := (others => '0');
   signal wraddress : std_logic_vector(1 downto 0) := (others => '0');
   signal wren : std_logic := '0';

    --Outputs
   signal q : std_logic_vector(5 downto 0);

   -- Clock period definitions
   constant CLK_period : time := 1000 ns;
 
BEGIN
 
    -- Instantiate the Unit Under Test (UUT)
   uut: page_reg_4x6_lpm PORT MAP (
          CLK => CLK,
          RSTn => RSTn,
          data => data,
          rdaddress => rdaddress,
          wraddress => wraddress,
          wren => wren,
          q => q
        );

   -- Clock process definitions
   CLK_process :process
   begin
        CLK <= '0';
        wait for CLK_period/2;
        CLK <= '1';
        wait for CLK_period/2;
   end process;
 

   -- Stimulus process
   stim_proc: process
      variable err_cnt: integer := 0;
   begin		
      -- hold reset state for 100 ns.
      wait for 100 ns;	

      wait for CLK_period*10;

      RSTn <= '0';

      -- case 1
      wait for CLK_period*4;

      data <= "101010";
      rdaddress <= "00";
      wraddress <= "00";
      wren <= '0';

      wait for CLK_period*2;
      assert (q="000000") report "Test1 Failed!" severity error;
      if (q/="000000") then
         err_cnt := err_cnt+1;
      end if;


      RSTn <= '1';

	  -- case 2
	  wait for CLK_period*2;

      data <= "101010";
      rdaddress <= "00";
      wraddress <= "00";

      wren <= '1';

	  wait for CLK_period*2;
	  assert (q="101010") report "Test2 Failed!" severity error;
	  if (q/=data) then
	     err_cnt := err_cnt+1;
	  end if;	


	  -- case 3
	  wait for CLK_period*2;

      data <= "111010";
      rdaddress <= "00";
      wraddress <= "01";

      wren <= '1';

	  wait for CLK_period*2;
	  assert (q="101010") report "Test2 Failed!" severity error;
	  if (q/=data) then
	     err_cnt := err_cnt+1;
	  end if;	


	  -- case 4
	  wait for CLK_period*2;

      data <= "101010";
      rdaddress <= "01";
      wraddress <= "00";

      wren <= '0';

	  wait for CLK_period*2;
	  assert (q="111010") report "Test2 Failed!" severity error;
	  if (q/=data) then
	     err_cnt := err_cnt+1;
	  end if;	

      -- insert stimulus here

      wait;
   end process;

END;
