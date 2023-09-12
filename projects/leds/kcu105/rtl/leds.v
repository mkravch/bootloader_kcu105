`timescale 1 ns/1 ps
module leds
    (
		
        input clk_300_p_i,
        input clk_300_n_i,
        output reg [7:0] leds_o = 8'd1

    ) /* synthesis syn_noclockbuf=1 */;
	
	wire clk_300;
	
	IBUFDS m_clk_300
    (
        .I(clk_300_p_i),
        .IB(clk_300_n_i),
        .O(clk_300)
    );


    reg [29:0] cnt = 0;
    reg [2:0] led_cnt = 0;
    
    reg [29:0] cnt_limit = 30'd99_999_999;
    

	always @ (posedge clk_300)
	begin
	
        if (cnt == cnt_limit)
            begin
                cnt <= 30'd0;
                led_cnt <= led_cnt + 3'd1;
                leds_o <= {leds_o[6:0],leds_o[7]};
                
            
            end
        else
            begin
                cnt <= cnt + 30'd1;
            end
	
	end
	
	reg [2:0] speed = 0;
	
	reg state = 0;
	
	always @ (posedge clk_300)
	begin
        if ( state == 0)
            begin
                if (leds_o[7])
                begin
                    state <= 1'b1;
                    speed <= speed + 3'd1;
                end
            
            end
        else
            begin
                if (~leds_o[7])
                    state <= 1'b0;
            end
	end
	
	always @ (posedge clk_300)
	case(speed)
	   3'd0: cnt_limit <= 30'd99_999_999;
	   3'd1: cnt_limit <= 30'd49_999_999;
	   3'd2: cnt_limit <= 30'd24_999_999;
	   3'd3: cnt_limit <= 30'd13_999_999;
	   3'd4: cnt_limit <= 30'd10_999_999;
	   3'd5: cnt_limit <= 30'd7_999_999;
	   3'd6: cnt_limit <= 30'd2_999_999;
	   3'd7: cnt_limit <= 30'd999_999;
	endcase
		
	
endmodule 


