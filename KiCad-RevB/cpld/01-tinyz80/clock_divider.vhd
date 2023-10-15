----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    09:39:03 09/08/2023 
-- Design Name: 
-- Module Name:    clock_divider - Behavioral 
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
use IEEE.numeric_std.ALL;

-- Uncomment the following library declaration if using
-- arithmetic functions with Signed or Unsigned values
--use IEEE.NUMERIC_STD.ALL;

-- Uncomment the following library declaration if instantiating
-- any Xilinx primitives in this code.
--library UNISIM;
--use UNISIM.VComponents.all;

entity CLOCK_DIVIDER is
    port (
        CLK, RSTn : in std_logic;
        Q : out std_logic);
end CLOCK_DIVIDER;

architecture Behavioral of CLOCK_DIVIDER is
    signal count: integer := 1;
    signal tmp : std_logic := '0';
begin
    process(CLK, RSTn)
    begin
        if(RSTn = '0') then
            count <= 1;
            tmp <= '0';
        elsif(CLK'event and CLK='1') then
            count <= count + 1;
            if (count = 10000) then
                tmp <= NOT tmp;
                count <= 1;
            end if;
        end if;
        Q <= tmp;
    end process;
end Behavioral;

