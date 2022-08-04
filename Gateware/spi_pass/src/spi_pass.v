// ice-v spi passthru design for PSRAM access
// 06-22-22 E. Brombaugh

`default_nettype none

module spi_pass(
	// 12MHz external osc
	input clk_12MHz,
		
	// RISCV SPI
	output	spi0_mosi,		// SPI core 0
	input	spi0_miso,
	output	spi0_sclk,
			spi0_cs0,
	output	spi0_nwp,
			spi0_nhld,
			
	// RGB output
    output RGB0, RGB1, RGB2, // RGB LED outs
	
	// SPI slave port
	input SPI_CSL,
	input SPI_MOSI,
	output SPI_MISO,
	input SPI_SCLK
);
	//------------------------------
	// SPI pass-thru
	//------------------------------
	assign spi0_mosi = SPI_MOSI;
	assign SPI_MISO = spi0_miso;
	assign spi0_sclk = SPI_SCLK;
	assign spi0_cs0 = SPI_CSL;
	assign spi0_nwp = 1'b1;
	assign spi0_nhld = 1'b1;
	
	//------------------------------
	// Clock PLL
	//------------------------------
	// Fin=12, FoutA=24, FoutB=48
	wire clk, pll_lock;
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
		.PLLOUTGLOBALA(),
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
	reg reset;    
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
	
	//------------------------------
	// LED counter
	//------------------------------
	reg [31:0] count;
	always @(posedge clk)
		if(reset)
			count <= 32'd0;
		else
			count <= count + 32'd1;
	
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
		.RGBLEDEN(1'b1),
		.RGB0PWM(count[23]),
		.RGB1PWM(count[24]),
		.RGB2PWM(count[25]),
		.RGB0(RGB0),
		.RGB1(RGB1),
		.RGB2(RGB2)
	);

endmodule
