// Manufacturing Test design for ICE-V Wireless FPGA
// 07-27-22 E. Brombaugh

`default_nettype none

module mfgtst(
	// 12MHz external osc
	input clk_12MHz,
	
	// PSRAM SPI port
	input spi0_miso,
	output spi0_sclk,
	output spi0_cs0,
	output spi0_mosi,
	output spi0_nwp,
	output spi0_nhld,

	// PMODs rigged as in/out lashed together for continuity tests
	input PMOD1_1,
	output PMOD1_2,
	input PMOD1_3,
	output PMOD1_4,
	input PMOD1_5,
	output PMOD1_6,
	input PMOD1_7,
	output PMOD1_8,

	input PMOD2_1,
	output PMOD2_2,
	input PMOD2_3,
	output PMOD2_4,
	input PMOD2_5,
	output PMOD2_6,
	input PMOD2_7,
	output PMOD2_8,

	input PMOD3_1,
	output PMOD3_2,
	input PMOD3_3,
	output PMOD3_4,
	input PMOD3_5,
	output PMOD3_6,
	input PMOD3_7,
	output PMOD3_8,

	// Lone FPGA on J3 - lashed to a C3 GPIO for testing
	output J3_11,
	
	// RGB output
    output RGB0, RGB1, RGB2, // RGB LED outs
	
	// SPI slave port
	input SPI_CSL,
	input SPI_MOSI,
	output SPI_MISO,
	input SPI_SCLK
);
	// This should be unique so firmware knows who it's talking to
	parameter DESIGN_ID = 32'hFEDC3210;

	//------------------------------
	// Clock PLL
	//------------------------------
	// Fin=12, FoutA=24, FoutB=48
	wire clk, clk24, pll_lock;
	SB_PLL40_2F_PAD #(
		.DIVR(4'b0000),
		.DIVF(7'b0111111),	// 24/48
		.DIVQ(3'b100),
		.FILTER_RANGE(3'b001),
		.FEEDBACK_PATH("SIMPLE"),
		.DELAY_ADJUSTMENT_MODE_FEEDBACK("FIXED"),
		.FDA_FEEDBACK(4'b0000),
		.DELAY_ADJUSTMENT_MODE_RELATIVE("FIXED"),
		.FDA_RELATIVE(4'b0000),
		.SHIFTREG_DIV_MODE(2'b00),
		.PLLOUT_SELECT_PORTA("GENCLK_HALF"),
		.PLLOUT_SELECT_PORTB("GENCLK"),
		.ENABLE_ICEGATE_PORTA(1'b0),
		.ENABLE_ICEGATE_PORTB(1'b0)
	)
	pll_inst (
		.PACKAGEPIN(clk_12MHz),
		.PLLOUTCOREA(),
		.PLLOUTGLOBALA(clk24),
		.PLLOUTCOREB(),
		.PLLOUTGLOBALB(clk),
		.EXTFEEDBACK(),
		.DYNAMICDELAY(8'h00),
		.RESETB(1'b1),
		.BYPASS(1'b0),
		.LATCHINPUTVALUE(),
		.LOCK(pll_lock),
		.SDI(),
		.SDO(),
		.SCLK()
	);
	
	//------------------------------
	// reset generator waits > 10us
	//------------------------------
	reg [9:0] reset_cnt;
	reg reset, reset24;    
	always @(posedge clk or negedge pll_lock)
	begin
		if(!pll_lock)
		begin
			reset_cnt <= 10'h000;
			reset <= 1'b1;
		end
		else
		begin
			if(reset_cnt != 10'h3ff)
			begin
				reset_cnt <= reset_cnt + 10'h001;
				reset <= 1'b1;
			end
			else
				reset <= 1'b0;
		end
	end
	
	always @(posedge clk24 or negedge pll_lock)
		if(!pll_lock)
			reset24 <= 1'b1;
		else
			reset24 <= reset;
	
	//------------------------------
	// 10kHz Internal LF OSC
	//------------------------------
	wire clk_lf;
	SB_LFOSC ulfosc(
		.CLKLFPU(1'b1),
		.CLKLFEN(1'b1),
		.CLKLF(clk_lf)
	);
		
	//------------------------------
	// Sync 10kHz rise edge to 48MHz 
	//------------------------------
	reg [2:0] sync_lf;
	reg samp_lf;
	always @(posedge clk)
		if(reset)
		begin
			sync_lf <= 3'd0;
			samp_lf <= 1'b0;
		end
		else
		begin
			sync_lf <= {sync_lf[1:0],clk_lf};
			samp_lf <= ~sync_lf[2] & sync_lf[1];
		end
	
	//--------------------------------
	// Count 48MHz clocks and
	// Dump counter on rise of 10kHz
	//--------------------------------
	reg [12:0] clkcnt, clkhld;
	always @(posedge clk)
		if(reset)
		begin
			clkcnt <= 13'd0;
			clkhld <= 13'd0;
		end
		else
		begin
			if(samp_lf)
			begin
				clkcnt <= 13'd0;
				clkhld <= clkcnt;
			end
			else
				clkcnt <= clkcnt + 13'd1;
		end
	
	//------------------------------
	// Internal SPI slave port
	//------------------------------
	wire [31:0] wdat;
	reg [31:0] rdat;
	wire [6:0] addr;
	wire re, we;
	spi_slave
		uspi(.clk(clk), .reset(reset),
			.spiclk(SPI_SCLK), .spimosi(SPI_MOSI),
			.spimiso(SPI_MISO), .spicsl(SPI_CSL),
			.we(we), .re(re), .wdat(wdat), .addr(addr), .rdat(rdat));
	
	//------------------------------
	// Writeable registers
	//------------------------------
	reg [3:0] per;
	reg [23:0] rgb;
	reg [12:0] gp_out;
	always @(posedge clk)
	begin
		if(reset)
		begin
			per <= 4'h4;
			rgb <= 24'h0;
			gp_out <= 13'h0;
		end
		else if(we)
		begin
			case(addr)
				7'h01: per <= wdat;
				7'h02: rgb <= wdat;
				7'h03: gp_out <= wdat;
			endcase
		end
	end
	
	//------------------------------------------
	// Hookup Odd/Even GPIO for continuity tests
	// with offset to help find shorts on
	// pkg adjacenct pins
	// 1<-4
	// 3<-6
	// 5<-8
	// 7<-2
	//------------------------------------------
	
	//------------------------------
	// GP Output to even PMOD pins 
	//------------------------------
	assign {J3_11,PMOD3_2,PMOD3_8,PMOD3_6,PMOD3_4,PMOD2_2,PMOD2_8,PMOD2_6,PMOD2_4,PMOD1_2,PMOD1_8,PMOD1_6,PMOD1_4} = gp_out;
	
	//------------------------------
	// GP Input from Odd PMOD pins
	//------------------------------
	wire [31:0] gp_in = {PMOD3_7,PMOD3_5,PMOD3_3,PMOD3_1,PMOD2_7,PMOD2_5,PMOD2_3,PMOD2_1,PMOD1_7,PMOD1_5,PMOD1_3,PMOD1_1};

	//------------------------------
	// readback
	//------------------------------
	always @(*)
	begin
		case(addr)
			7'h00: rdat = DESIGN_ID;
			7'h01: rdat = per;
			7'h02: rdat = rgb;
			7'h03: rdat = gp_out;
			7'h04: rdat = gp_in;
			7'h05: rdat = clkhld;
			default: rdat = 32'd0;
		endcase
	end
	
	//------------------------------
	// Tie off the PSRAM SPI
	//------------------------------
	assign spi0_sclk = 1'b1;
	assign spi0_cs0 = 1'b1;
	assign spi0_mosi = 1'b1;
	assign spi0_nwp = 1'b1;
	assign spi0_nhld = 1'b1;

	//------------------------------
	// PWM dimming for the RGB DRV 
	//------------------------------
	reg [11:0] pwm_cnt;
	reg led_ena, r_pwm, g_pwm, b_pwm;
	always @(posedge clk)
		if(reset)
		begin
			pwm_cnt <= 8'd0;
			led_ena <= 1'b1;
		end
		else
		begin
			pwm_cnt <= pwm_cnt + 1;
			led_ena <= pwm_cnt[3:0] <= per;
			r_pwm <= pwm_cnt[11:4] < rgb[23:16];
			g_pwm <= pwm_cnt[11:4] < rgb[15:8];
			b_pwm <= pwm_cnt[11:4] < rgb[7:0];
		end
	
	//------------------------------
	// Instantiate RGB DRV 
	//------------------------------
	SB_RGBA_DRV #(
		.CURRENT_MODE("0b1"),
		.RGB0_CURRENT("0b000001"),
		.RGB1_CURRENT("0b000001"),
		.RGB2_CURRENT("0b000001")
	) RGBA_DRIVER (
		.CURREN(1'b1),
		.RGBLEDEN(led_ena),
		.RGB0PWM(r_pwm),
		.RGB1PWM(g_pwm),
		.RGB2PWM(b_pwm),
		.RGB0(RGB0),
		.RGB1(RGB1),
		.RGB2(RGB2)
	);
endmodule
