----------------------------------------------------------------------------------
-- Company: 
-- Engineer: 
-- 
-- Create Date:    13:10:55 07/30/2023 
-- Design Name: 
-- Module Name:    page_reg_4x6_lpm - Behavioral 
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

entity page_reg_4x6_lpm is
    port
    (
        CLK : in std_logic;
        RSTn : in std_logic;
        data : IN STD_LOGIC_VECTOR (5 DOWNTO 0);
        rdaddress : IN STD_LOGIC_VECTOR (1 DOWNTO 0);
        wraddress : IN STD_LOGIC_VECTOR (1 DOWNTO 0);
        wren : IN STD_LOGIC  := '0';
        q : OUT STD_LOGIC_VECTOR (5 DOWNTO 0)
	);
end page_reg_4x6_lpm;

architecture Behavioral of page_reg_4x6_lpm is
   SIGNAL sub_wire0	: STD_LOGIC_VECTOR (5 DOWNTO 0);
   SIGNAL sub_wire1	: STD_LOGIC_VECTOR (5 DOWNTO 0);
   SIGNAL sub_wire2	: STD_LOGIC_VECTOR (5 DOWNTO 0);
   SIGNAL sub_wire3	: STD_LOGIC_VECTOR (5 DOWNTO 0);
--   SIGNAL sub_wire_d: STD_LOGIC_VECTOR (5 DOWNTO 0);
   
--	component altdpram
--	port (
--			data	: IN STD_LOGIC_VECTOR (5 DOWNTO 0);
--			rdaddress	: IN STD_LOGIC_VECTOR (1 DOWNTO 0);
--			wraddress	: IN STD_LOGIC_VECTOR (1 DOWNTO 0);
--			wren	: IN STD_LOGIC ;
--			q	: OUT STD_LOGIC_VECTOR (5 DOWNTO 0)
--	);
--	end component;
begin
   -- q    <= sub_wire0(5 DOWNTO 0);
--	altdpram_component : altdpram
--	port map (
--		data => data,
--		rdaddress => rdaddress,
--		wraddress => wraddress,
--		wren => wren,
--		q => sub_wire0
--	);

--    q <= sub_wire0(5 DOWNTO 0) when rdaddress = b"00" and RSTn = '1' else
--         sub_wire1(5 DOWNTO 0) when rdaddress = b"01" and RSTn = '1' else
--         sub_wire2(5 DOWNTO 0) when rdaddress = b"10" and RSTn = '1' else
--         sub_wire3(5 DOWNTO 0) when rdaddress = b"11" and RSTn = '1' else
--         b"000000";

    q <= sub_wire0(5 DOWNTO 0) when rdaddress = "00" else
         sub_wire1(5 DOWNTO 0) when rdaddress = "01" else
         sub_wire2(5 DOWNTO 0) when rdaddress = "10" else
         sub_wire3(5 DOWNTO 0) when rdaddress = "11";

    process(CLK, RSTn)
    begin
        if rising_edge(CLK) then
            if RSTn = '0' then
                sub_wire0 <= (others => '0');
                sub_wire1 <= (others => '0');
                sub_wire2 <= (others => '0');
                sub_wire3 <= (others => '0');
                --sub_wire_d <= (others => '0');
                -- q <= b"000000";
                -- q <= sub_wire_d;
            else
                if wren = '1' then
                    if wraddress = "00" then
                        sub_wire0 <= data;
                    elsif wraddress = "01" then
                        sub_wire1 <= data;
                    elsif wraddress = "10" then 
                        sub_wire2 <= data;
                    elsif wraddress = "11" then 
                        sub_wire3 <= data;
                    end if;
                end if;

--                if rdaddress = "00" then
--                    q <= sub_wire0;
--                elsif rdaddress = "01" then
--                    q <= sub_wire1;
--                elsif rdaddress = "10" then 
--                    q <= sub_wire2;
--                elsif rdaddress = "11" then 
--                    q <= sub_wire3;
--                end if;


    --            sub_wire_d <= data;
    --            q <= sub_wire_d;
            end if;
--            q <= sub_wire0(5 DOWNTO 0) when rdaddress = "00" else
--                 sub_wire1(5 DOWNTO 0) when rdaddress = "01" else
--                 sub_wire2(5 DOWNTO 0) when rdaddress = "10" else
--                 sub_wire3(5 DOWNTO 0) when rdaddress = "11";
            --q <= b"101010";
        end if;
    end process;
end Behavioral;

