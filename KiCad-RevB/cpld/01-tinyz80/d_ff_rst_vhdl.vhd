----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    13:08:54 07/30/2023 
-- Design Name: 
-- Module Name:    D_FF_RST - Behavioral 
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

entity D_FF_RST is
    port
    (
        CLK : in std_logic;
        RSTn : in std_logic;
        D : in std_logic;
        Q : out std_logic
    );
end D_FF_RST;

architecture Behavioral of D_FF_RST is
begin
    process (CLK, RSTn) is
    begin
        if RSTn = '0' then
            Q <= '0';
        elsif rising_edge(CLK) then  
            Q <= D;
        end if;
    end process;
end Behavioral;
