/////////////////////////////////////////////////
////////////	LATTICE BOLTZMANN	/////////////
/////////////////////////////////////////////////
// V. Hunter Adams
// vha3@cornell.edu
// 5/23/2023
/////////////////////////////////////////////////

module DDS ( 	input wire clock,
				input wire memclock,
				input wire reset,
				input wire [9:0] next_x,
				input wire [9:0] next_y,
				input wire [9:0] color_shift,
				output wire [7:0] color_out);

	// Number of rows/columns (zero-indexed)
	parameter num_rows = 5'd31 ;
	parameter num_cols = 9'd511 ;

	parameter naught_init = 20'sh_e024 ;
	parameter north_init = 20'sh_3808 ;
	parameter south_init = 20'sh_3808 ;
	parameter east_init = 20'sh_4ba8 ;
	parameter west_init = 20'sh_2986 ;
	parameter northeast_init = 20'sh_12e9 ;
	parameter northwest_init = 20'sh_a61 ;
	parameter southeast_init = 20'sh_12e9 ;
	parameter southwest_init = 20'sh_a61 ;

	// One, 1/9, 1/36 in fixed point
	parameter fixone  = 20'sh_20000 ;
	parameter one9th  = 20'sh_38e3 ;
	parameter one36th = 20'sh_e38 ;

	parameter DW = 19 ;

	// M10k control lines
	reg signed 	[DW:0] 	north_data_in 				[num_rows:0];
	reg 		[8:0]	north_write_address 		[num_rows:0];
	reg 		[8:0]	north_next_write_address 	[num_rows:0];
	reg 		[8:0]	north_read_address 			[num_rows:0];
	wire signed [DW:0]	north_data_out 				[num_rows:0];
	reg 				north_write_enable 			[num_rows:0];

	reg signed 	[DW:0] 	south_data_in 				[num_rows:0];
	reg 		[8:0]	south_write_address 		[num_rows:0];
	reg 		[8:0]	south_next_write_address 	[num_rows:0];
	reg 		[8:0]	south_read_address 			[num_rows:0];
	wire signed [DW:0]	south_data_out 				[num_rows:0];
	reg 				south_write_enable 			[num_rows:0];

	reg signed 	[DW:0] 	east_data_in 				[num_rows:0];
	reg 		[8:0]	east_write_address 			[num_rows:0];
	reg 		[8:0]	east_next_write_address 	[num_rows:0];
	reg 		[8:0]	east_read_address 			[num_rows:0];
	wire signed [DW:0]	east_data_out 				[num_rows:0];
	reg 				east_write_enable 			[num_rows:0];

	reg signed 	[DW:0] 	west_data_in 				[num_rows:0];
	reg 		[8:0]	west_write_address 			[num_rows:0];
	reg 		[8:0]	west_next_write_address 	[num_rows:0];
	reg 		[8:0]	west_read_address 			[num_rows:0];
	wire signed [DW:0]	west_data_out 				[num_rows:0];
	reg 				west_write_enable 			[num_rows:0];

	reg signed 	[DW:0] 	northeast_data_in 			[num_rows:0];
	reg 		[8:0]	northeast_write_address 	[num_rows:0];
	reg 		[8:0]	northeast_next_write_address[num_rows:0];
	reg 		[8:0]	northeast_read_address 		[num_rows:0];
	wire signed [DW:0]	northeast_data_out 			[num_rows:0];
	reg 				northeast_write_enable 		[num_rows:0];

	reg signed 	[DW:0] 	northwest_data_in 			[num_rows:0];
	reg 		[8:0]	northwest_write_address 	[num_rows:0];
	reg 		[8:0]	northwest_next_write_address[num_rows:0];
	reg 		[8:0]	northwest_read_address 		[num_rows:0];
	wire signed [DW:0]	northwest_data_out 			[num_rows:0];
	reg 				northwest_write_enable 		[num_rows:0];

	reg signed 	[DW:0] 	southeast_data_in 			[num_rows:0];
	reg 		[8:0]	southeast_write_address 	[num_rows:0];
	reg 		[8:0]	southeast_next_write_address[num_rows:0];
	reg 		[8:0]	southeast_read_address 		[num_rows:0];
	wire signed [DW:0]	southeast_data_out 			[num_rows:0];
	reg 				southeast_write_enable 		[num_rows:0];

	reg signed 	[DW:0] 	southwest_data_in 			[num_rows:0];
	reg 		[8:0]	southwest_write_address 	[num_rows:0];
	reg 		[8:0]	southwest_next_write_address[num_rows:0];
	reg 		[8:0]	southwest_read_address 		[num_rows:0];
	wire signed [DW:0]	southwest_data_out 			[num_rows:0];
	reg 				southwest_write_enable 		[num_rows:0];

	reg signed 	[DW:0] 	naught_data_in 				[num_rows:0];
	reg 		[8:0]	naught_write_address 		[num_rows:0];
	reg 		[8:0]	naught_next_write_address 	[num_rows:0];
	reg 		[8:0]	naught_read_address 		[num_rows:0];
	wire signed [DW:0]	naught_data_out 			[num_rows:0];
	reg 				naught_write_enable 		[num_rows:0];

	reg signed 	[DW:0] 	barrier_data_in 			[num_rows:0];
	reg 		[8:0]	barrier_write_address 		[num_rows:0];
	reg 		[8:0]	barrier_next_write_address 	[num_rows:0];
	reg 		[8:0]	barrier_read_address 		[num_rows:0];
	wire signed [DW:0]	barrier_data_out 			[num_rows:0];
	reg 				barrier_write_enable 		[num_rows:0];

	reg signed 	[DW:0] 	speed_data_in 				[num_rows:0];
	reg 		[8:0]	speed_write_address 		[num_rows:0];
	reg 		[8:0]	speed_next_write_address 	[num_rows:0];
	reg 		[8:0]	speed_read_address 			[num_rows:0];
	wire signed [DW:0]	speed_data_out 				[num_rows:0];
	reg 				speed_write_enable 			[num_rows:0];


	assign color_out = (next_y < 32) ? ((next_x < 512) ? ((speed_data_out[next_y[4:0]]>>>color_shift) & 8'b_111_111_11 ) : 8'd_0 ) : 8'd_0 ;


	/////////////////////////////////////////////////
	////////////	Initializations 	/////////////
	/////////////////////////////////////////////////
	reg signed 	[DW:0] 			counter	[num_rows:0] ;
	reg 		[num_rows:0] 	state ;
	parameter 	init_done = 32'h_FFFF_FFFF ; // THIS NEEDS TO CHANGE AS YOU ADD MORE ROWS


	/////////////////////////////////////////////////
	////////////		Streaming	 	/////////////
	/////////////////////////////////////////////////
	// State register
	reg stream_state = 1'b_0 ;

	// Pointers
	reg [4:0] north_M10k_base 					[num_rows:0] ;
	reg [8:0] north_address_base 				[num_rows:0] ;
	reg [4:0] south_M10k_base 					[num_rows:0] ;
	reg [8:0] south_address_base 				[num_rows:0] ;
	reg [4:0] east_M10k_base 					[num_rows:0] ;
	reg [8:0] east_address_base 				[num_rows:0] ;
	reg [4:0] west_M10k_base 					[num_rows:0] ;
	reg [8:0] west_address_base 				[num_rows:0] ;
	reg [4:0] northeast_M10k_base 				[num_rows:0] ;
	reg [8:0] northeast_address_base 			[num_rows:0] ;
	reg [4:0] northwest_M10k_base 				[num_rows:0] ;
	reg [8:0] northwest_address_base 			[num_rows:0] ;
	reg [4:0] southeast_M10k_base 				[num_rows:0] ;
	reg [8:0] southeast_address_base 			[num_rows:0] ;
	reg [4:0] southwest_M10k_base 				[num_rows:0] ;
	reg [8:0] southwest_address_base 			[num_rows:0] ;
	reg [4:0] naught_M10k_base 					[num_rows:0] ;
	reg [8:0] naught_address_base 				[num_rows:0] ;
	reg [4:0] barrier_M10k_base 				[num_rows:0] ;
	reg [8:0] barrier_address_base 				[num_rows:0] ;
	reg [4:0] speed_M10k_base					[num_rows:0] ;
	reg [8:0] speed_address_base 				[num_rows:0] ;


	/////////////////////////////////////////////////
	////////////		Bouncing	 	/////////////
	/////////////////////////////////////////////////
	reg 	[7:0]			bounce_state [num_rows:0] 	;
	reg 	[num_rows:0]	bounce_done 				;

	// For addressing into M10k blocks
	reg [8:0] north_looping_base 		[num_rows:0] ;
	reg [8:0] south_looping_base 		[num_rows:0] ;
	reg [8:0] east_looping_base 		[num_rows:0] ;
	reg [8:0] west_looping_base 		[num_rows:0] ;
	reg [8:0] northeast_looping_base 	[num_rows:0] ;
	reg [8:0] northwest_looping_base 	[num_rows:0] ;
	reg [8:0] southeast_looping_base 	[num_rows:0] ;
	reg [8:0] southwest_looping_base 	[num_rows:0] ;
	reg [8:0] naught_looping_base 		[num_rows:0] ;
	reg [8:0] barrier_looping_base 		[num_rows:0] ;

	// Registers for combinational update
	reg signed 	[DW:0] 	n0 				[num_rows:0] ;
	reg signed 	[DW:0]	nN 				[num_rows:0] ;
	reg signed 	[DW:0] 	nE 				[num_rows:0] ;
	reg signed 	[DW:0] 	nW 				[num_rows:0] ;
	reg signed 	[DW:0]	nS 				[num_rows:0] ;
	reg signed 	[DW:0] 	nNE 			[num_rows:0] ;
	reg signed 	[DW:0] 	nNW 			[num_rows:0] ;
	reg signed 	[DW:0]	nSE 			[num_rows:0] ;
	reg signed 	[DW:0] 	nSW 			[num_rows:0] ;

	reg signed 	[DW:0]	nNm 			[num_rows:0] ;
	reg signed 	[DW:0] 	nEm 			[num_rows:0] ;
	reg signed 	[DW:0] 	nWm 			[num_rows:0] ;
	reg signed 	[DW:0]	nSm 			[num_rows:0] ;
	reg signed 	[DW:0] 	nNEm 			[num_rows:0] ;
	reg signed 	[DW:0] 	nNWm 			[num_rows:0] ;
	reg signed 	[DW:0]	nSEm 			[num_rows:0] ;
	reg signed 	[DW:0] 	nSWm 			[num_rows:0] ;

	wire signed [DW:0] 	n0_new 			[num_rows:0] ;
	wire signed [DW:0]	nN_new 			[num_rows:0] ;
	wire signed [DW:0] 	nE_new 			[num_rows:0] ;
	wire signed [DW:0] 	nW_new 			[num_rows:0] ;
	wire signed [DW:0]	nS_new 			[num_rows:0] ;
	wire signed [DW:0] 	nNE_new 		[num_rows:0] ;
	wire signed [DW:0] 	nNW_new 		[num_rows:0] ;
	wire signed [DW:0]	nSE_new 		[num_rows:0] ;
	wire signed [DW:0] 	nSW_new 		[num_rows:0] ;

	reg signed 	[DW:0] 	rho 			[num_rows:0] ;
	reg signed 	[DW:0] 	rho_m_1 		[num_rows:0] ;
	wire signed [DW:0]  rho_wire 		[num_rows:0] ;
	reg signed 	[DW:0] 	one9th_rho 		[num_rows:0] ;
	reg signed 	[DW:0] 	one36th_rho 	[num_rows:0] ;
	reg signed 	[DW:0] 	rho_inv 		[num_rows:0] ;

	wire signed [DW:0]  ux_num	 		[num_rows:0] ;
	wire signed [DW:0]  uy_num	 		[num_rows:0] ;
	reg signed 	[DW:0] 	ux 				[num_rows:0] ;
	reg signed 	[DW:0] 	uy 				[num_rows:0] ;
	wire signed [DW:0] 	vx3 			[num_rows:0] ;
	wire signed [DW:0] 	vy3 			[num_rows:0] ;
	reg signed 	[DW:0] 	vx2 			[num_rows:0] ;
	reg signed 	[DW:0] 	vy2 			[num_rows:0] ;
	reg signed 	[DW:0] 	vxvy2 			[num_rows:0] ;
	wire signed [DW:0] 	v2  			[num_rows:0] ;
	wire signed [DW:0] 	v215 			[num_rows:0] ;
	wire signed [DW:0] 	vx2_4_5 		[num_rows:0] ;
	wire signed [DW:0] 	vy2_4_5 		[num_rows:0] ;
	wire signed [DW:0] 	v2_m_2vxvy_4_5	[num_rows:0] ;
	wire signed [DW:0] 	v2_p_2vxvy_4_5 	[num_rows:0] ;

	// Multiplier inputs
	reg signed 	[DW:0] 	mult_1_in_1 		[num_rows:0] ;
	reg signed 	[DW:0] 	mult_1_in_2 		[num_rows:0] ;
	wire signed [DW:0] 	mult_1_in_1_wire 	[num_rows:0] ;
	wire signed [DW:0] 	mult_1_in_2_wire 	[num_rows:0] ;
	wire signed [DW:0]  mult_1_out 			[num_rows:0] ;

	reg signed 	[DW:0] 	mult_2_in_1 		[num_rows:0] ;
	reg signed 	[DW:0] 	mult_2_in_2 		[num_rows:0] ;
	wire signed [DW:0] 	mult_2_in_1_wire 	[num_rows:0] ;
	wire signed [DW:0] 	mult_2_in_2_wire 	[num_rows:0] ;
	wire signed [DW:0]  mult_2_out 			[num_rows:0] ;


	generate
		genvar j;
			for (j=0; j<=num_rows; j=j+1) begin: bounceGenerator

				// Build a few M10k blocks. One for each direction, one for barriers, and one for speed.
				M10K_512_18 north (	.q(north_data_out[j]),
									.d(north_data_in[j]),
									.write_address(north_write_address[j]),
									.read_address(north_read_address[j]),
									.we(north_write_enable[j]), 
									.clk(clock)
								) ;
				M10K_512_18 south (	.q(south_data_out[j]),
									.d(south_data_in[j]),
									.write_address(south_write_address[j]),
									.read_address(south_read_address[j]),
									.we(south_write_enable[j]), 
									.clk(clock)
								) ;
				M10K_512_18 east (	.q(east_data_out[j]),
									.d(east_data_in[j]),
									.write_address(east_write_address[j]),
									.read_address(east_read_address[j]),
									.we(east_write_enable[j]), 
									.clk(clock)
								) ;
				M10K_512_18 west (	.q(west_data_out[j]),
									.d(west_data_in[j]),
									.write_address(west_write_address[j]),
									.read_address(west_read_address[j]),
									.we(west_write_enable[j]), 
									.clk(clock)
								) ;
				M10K_512_18 northeast (	.q(northeast_data_out[j]),
										.d(northeast_data_in[j]),
										.write_address(northeast_write_address[j]),
										.read_address(northeast_read_address[j]),
										.we(northeast_write_enable[j]), 
										.clk(clock)
									) ;
				M10K_512_18 northwest (	.q(northwest_data_out[j]),
										.d(northwest_data_in[j]),
										.write_address(northwest_write_address[j]),
										.read_address(northwest_read_address[j]),
										.we(northwest_write_enable[j]), 
										.clk(clock)
									) ;
				M10K_512_18 southeast (	.q(southeast_data_out[j]),
										.d(southeast_data_in[j]),
										.write_address(southeast_write_address[j]),
										.read_address(southeast_read_address[j]),
										.we(southeast_write_enable[j]), 
										.clk(clock)
									) ;
				M10K_512_18 southwest (	.q(southwest_data_out[j]),
										.d(southwest_data_in[j]),
										.write_address(southwest_write_address[j]),
										.read_address(southwest_read_address[j]),
										.we(southwest_write_enable[j]), 
										.clk(clock)
									) ;
				M10K_512_18 naught (	.q(naught_data_out[j]),
										.d(naught_data_in[j]),
										.write_address(naught_write_address[j]),
										.read_address(naught_read_address[j]),
										.we(naught_write_enable[j]), 
										.clk(clock)
									) ;
				M10K_512_18 barrier (	.q(barrier_data_out[j]),
										.d(barrier_data_in[j]),
										.write_address(barrier_write_address[j]),
										.read_address(barrier_read_address[j]),
										.we(barrier_write_enable[j]), 
										.clk(clock)
									) ;

				M10K_512_18 speed (		.q(speed_data_out[j]),
										.d(speed_data_in[j]),
										.write_address(speed_write_address[j]),
										// .read_address(speed_read_address[j]),
										.read_address(next_x[8:0]),
										.we(speed_write_enable[j]), 
										.clk(memclock)
								) ;

				// Combinational stuff
				assign rho_wire[j] = n0[j]+nN[j]+nE[j]+nW[j]+nS[j]+nNE[j]+nNW[j]+nSE[j]+nSW[j] ;
				assign ux_num[j] = nE[j] + nNE[j] + nSE[j] - nW[j] - nNW[j] - nSW[j] ;
				assign uy_num[j] = nN[j] + nNE[j] + nNW[j] - nS[j] - nSE[j] - nSW[j] ;
				assign vx3[j] = (ux[j] + (ux[j] <<< 1)) ;
				assign vy3[j] = (uy[j] + (uy[j] <<< 1)) ;
				assign v2[j] = (vx2[j] + vy2[j]) ;
				assign v215[j] = (vx2[j] + vy2[j]) + ((vx2[j] + vy2[j])>>>1) ;
				assign vx2_4_5[j] = (vx2[j] <<< 2) + (vx2[j] >>> 1) ;
				assign vy2_4_5[j] = (vy2[j] <<< 2) + (vy2[j] >>> 1) ;
				assign v2_m_2vxvy_4_5[j] = (((vx2[j] + vy2[j]) - vxvy2[j]) <<< 2) + (((vx2[j] + vy2[j]) - vxvy2[j]) >>> 1) ;
				assign v2_p_2vxvy_4_5[j] = (((vx2[j] + vy2[j]) + vxvy2[j]) <<< 2) + (((vx2[j] + vy2[j]) + vxvy2[j]) >>> 1) ;

				// Multiplication by omega
				assign nE_new[j]  = nE[j]  + (((nEm[j] - nE[j]) <<< 1) - ((nEm[j] - nE[j]) >>> 5)) ;
				assign nW_new[j]  = nW[j]  + (((nWm[j] - nW[j]) <<< 1) - ((nWm[j] - nW[j]) >>> 5)) ;
				assign nN_new[j]  = nN[j]  + (((nNm[j] - nN[j]) <<< 1) - ((nNm[j] - nN[j]) >>> 5)) ;
				assign nS_new[j]  = nS[j]  + (((nSm[j] - nS[j]) <<< 1) - ((nSm[j] - nS[j]) >>> 5)) ;
				assign nNE_new[j] = nNE[j] + (((nNEm[j] - nNE[j]) <<< 1) - ((nNEm[j] - nNE[j]) >>> 5)) ;
				assign nNW_new[j] = nNW[j] + (((nNWm[j] - nNW[j]) <<< 1) - ((nNWm[j] - nNW[j]) >>> 5)) ;
				assign nSE_new[j] = nSE[j] + (((nSEm[j] - nSE[j]) <<< 1) - ((nSEm[j] - nSE[j]) >>> 5)) ;
				assign nSW_new[j] = nSW[j] + (((nSWm[j] - nSW[j]) <<< 1) - ((nSWm[j] - nSW[j]) >>> 5)) ;

				assign n0_new[j] = rho[j] - (nE_new[j] + nW_new[j] + nN_new[j] + nS_new[j] + nNE_new[j] + nNW_new[j] + nSE_new[j] + nSW_new[j]) ;

				// Multipliers
				signed_mult MULT1 (	.out(mult_1_out[j]),
									.a(mult_1_in_1_wire[j]),
									.b(mult_1_in_2_wire[j])) ;
				signed_mult MULT2 (	.out(mult_2_out[j]),
									.a(mult_2_in_1_wire[j]),
									.b(mult_2_in_2_wire[j])) ;


				assign mult_1_in_1_wire[j] = mult_1_in_1[j] ;
				assign mult_1_in_2_wire[j] = mult_1_in_2[j] ;
				assign mult_2_in_1_wire[j] = mult_2_in_1[j] ;
				assign mult_2_in_2_wire[j] = mult_2_in_2[j] ;



				always @ (posedge clock) begin

					if (reset) begin
				  		counter[j] 						<= (20'd_0 + (20'd512*j)) ;
				  		// state[j] 						<= 1'b_0 ;
				  		north_next_write_address[j] 	<= 9'd_0 ;
				  		north_write_address[j] 			<= 9'd_0 ;
				  		south_next_write_address[j] 	<= 9'd_0 ;
				  		south_write_address[j] 			<= 9'd_0 ;
				  		east_next_write_address[j] 		<= 9'd_0 ;
				  		east_write_address[j] 			<= 9'd_0 ;
				  		west_next_write_address[j] 		<= 9'd_0 ;
				  		west_write_address[j] 			<= 9'd_0 ;
				  		northeast_next_write_address[j] <= 9'd_0 ;
				  		northeast_write_address[j] 		<= 9'd_0 ;
				  		northwest_next_write_address[j] <= 9'd_0 ;
				  		northwest_write_address[j] 		<= 9'd_0 ;
				  		southeast_next_write_address[j] <= 9'd_0 ;
				  		southeast_write_address[j] 		<= 9'd_0 ;
				  		southwest_next_write_address[j] <= 9'd_0 ;
				  		southwest_write_address[j] 		<= 9'd_0 ;
				  		naught_next_write_address[j] 	<= 9'd_0 ;
				  		naught_write_address[j] 		<= 9'd_0 ;
				  		barrier_next_write_address[j] 	<= 9'd_0 ;
				  		barrier_write_address[j] 		<= 9'd_0 ;
				  		speed_next_write_address[j] 	<= 9'd_0 ;
				  		speed_write_address[j] 			<= 9'd_0 ;

				  		// Streaming initializations
				  		north_M10k_base[j] <= 5'd_0 ;
						north_address_base[j] <= 9'd_0 ;
						south_M10k_base[j] <= 5'd_0 ;
						south_address_base[j] <= 9'd_0 ;
						east_M10k_base[j] <= 5'd_0 ;
						east_address_base[j] <= 9'd_0 ;
						west_M10k_base[j] <= 5'd_0 ;
						west_address_base[j] <= 9'd_0 ;
						northeast_M10k_base[j] <= 5'd_0 ;
						northeast_address_base[j] <= 9'd_0 ;
						northwest_M10k_base[j] <= 5'd_0 ;
						northwest_address_base[j] <= 9'd_0 ;
						southeast_M10k_base[j] <= 5'd_0 ;
						southeast_address_base[j] <= 9'd_0 ;
						southwest_M10k_base[j] <= 5'd_0 ;
						southwest_address_base[j] <= 9'd_0 ;
						naught_M10k_base[j] <= 5'd_0 ;
						naught_address_base[j] <= 9'd_0 ;
						barrier_M10k_base[j] <= 5'd_0 ;
						barrier_address_base[j] <= 9'd_0 ;
						speed_M10k_base[j] <= 5'd_0 ;
						speed_address_base[j] <= 9'd_0 ;

						// Bouncing initializations
						bounce_done[j] 		<= 1'b_0  ;
						bounce_state[j] 	<= 8'd_19 ;

				 	end
				  	else begin

				  		// INITIALIZATION
				  		if (bounce_state[j] == 8'd_19) begin

				  			// Increment counter
				  			counter[j] 						<= (counter[j] + 20'd_1) ;

				  			// Initialize the North M10k block for this row . . .
				  			north_write_enable[j]			<= (north_write_address[j] == num_cols) ? 1'b0 : 1'b1 ;
				  			north_data_in[j] 				<= north_init ; //(counter[j]) ; // North init will go here, a PIO port
				  			north_next_write_address[j]		<= (north_next_write_address[j] + 9'd_1) ;
				  			north_write_address[j] 			<= (north_next_write_address[j]) ;
				  			// Initialize the South M10k block for this row . . .
				  			south_write_enable[j]			<= (south_write_address[j] == num_cols) ? 1'b0 : 1'b1 ;
				  			south_data_in[j] 				<= south_init ; //(counter[j]) ; // south init will go here, a PIO port
				  			south_next_write_address[j]		<= (south_next_write_address[j] + 9'd_1) ;
				  			south_write_address[j] 			<= (south_next_write_address[j]) ;
				  			// Initialize the East M10k block for this row . . .
				  			east_write_enable[j]			<= (east_write_address[j] == num_cols) ? 1'b0 : 1'b1 ;
				  			east_data_in[j] 				<= east_init ; //(counter[j]) ; // east init will go here, a PIO port
				  			east_next_write_address[j]		<= (east_next_write_address[j] + 9'd_1) ;
				  			east_write_address[j] 			<= (east_next_write_address[j]) ;
				  			// Initialize the West M10k block for this row . . .
				  			west_write_enable[j]			<= (west_write_address[j] == num_cols) ? 1'b0 : 1'b1 ;
				  			west_data_in[j] 				<= west_init ; //(counter[j]) ; // west init will go here, a PIO port
				  			west_next_write_address[j]		<= (west_next_write_address[j] + 9'd_1) ;
				  			west_write_address[j] 			<= (west_next_write_address[j]) ;
				  			// Initialize the Northeast M10k block for this row . . .
				  			northeast_write_enable[j]		<= (northeast_write_address[j] == num_cols) ? 1'b0 : 1'b1 ;
				  			northeast_data_in[j] 			<= northeast_init ; //(counter[j]) ; // Northeast init will go here, a PIO port
				  			northeast_next_write_address[j]	<= (northeast_next_write_address[j] + 9'd_1) ;
				  			northeast_write_address[j] 		<= (northeast_next_write_address[j]) ;
				  			// Initialize the Northwest M10k block for this row . . .
				  			northwest_write_enable[j]		<= (northwest_write_address[j] == num_cols) ? 1'b0 : 1'b1 ;
				  			northwest_data_in[j] 			<= northwest_init ; //(counter[j]) ; // Northwest init will go here, a PIO port
				  			northwest_next_write_address[j]	<= (northwest_next_write_address[j] + 9'd_1) ;
				  			northwest_write_address[j] 		<= (northwest_next_write_address[j]) ;
				  			// Initialize the Southeast M10k block for this row . . .
				  			southeast_write_enable[j]		<= (southeast_write_address[j] == num_cols) ? 1'b0 : 1'b1 ;
				  			southeast_data_in[j] 			<= southeast_init ; //(counter[j]) ; // southeast init will go here, a PIO port
				  			southeast_next_write_address[j]	<= (southeast_next_write_address[j] + 9'd_1) ;
				  			southeast_write_address[j] 		<= (southeast_next_write_address[j]) ;
				  			// Initialize the Southwest M10k block for this row . . .
				  			southwest_write_enable[j]		<= (southwest_write_address[j] == num_cols) ? 1'b0 : 1'b1 ;
				  			southwest_data_in[j] 			<= southwest_init ; //(counter[j]) ; // southwest init will go here, a PIO port
				  			southwest_next_write_address[j]	<= (southwest_next_write_address[j] + 9'd_1) ;
				  			southwest_write_address[j] 		<= (southwest_next_write_address[j]) ;
				  			// Initialize the Naught M10k block for this row . . .
				  			naught_write_enable[j]			<= (naught_write_address[j] == num_cols) ? 1'b0 : 1'b1 ;
				  			naught_data_in[j] 				<= naught_init ; //(counter[j]) ; // naught init will go here, a PIO port
				  			naught_next_write_address[j]	<= (naught_next_write_address[j] + 9'd_1) ;
				  			naught_write_address[j] 		<= (naught_next_write_address[j]) ;
				  			// Initialize the Barrier M10k block for this row . . .
				  			barrier_write_enable[j]			<= (barrier_write_address[j] == num_cols) ? 1'b0 : 1'b1 ;
				  			barrier_data_in[j] 				<= (barrier_next_write_address[j] == 9'd_25) ? ((j>=11) ? ((j<=20) ? 20'd_1 : 20'd_0) : 20'd_0) : 20'd_0 ; 
				  			barrier_next_write_address[j]	<= (barrier_next_write_address[j] + 9'd_1) ;
				  			barrier_write_address[j] 		<= (barrier_next_write_address[j]) ;
				  			// Initialize the Speed M10k block for this row . . .
				  			speed_write_enable[j]			<= (speed_write_address[j] == num_cols) ? 1'b0 : 1'b1 ;
				  			speed_data_in[j] 				<= (20'd_0) ; // speed init will go here, a PIO port
				  			speed_next_write_address[j]		<= (speed_next_write_address[j] + 9'd_1) ;
				  			speed_write_address[j] 			<= (speed_next_write_address[j]) ;

				  			// Each controls one bit of the state register
				  			bounce_state[j] 				<= (north_write_address[j] == num_cols) ? 8'd_20 : 8'd_19 ;

				  		end

				  		//////////////////////// MODIFIED STREAMING ///////////////////////

				  		// Initialize all addresses (will all be zero)
						if (bounce_state[j] == 8'd_20) begin
							north_looping_base[j] 		<= north_address_base[j] ;
							south_looping_base[j] 		<= south_address_base[j] ;
							east_looping_base[j] 		<= east_address_base[j] ;
							west_looping_base[j] 		<= west_address_base[j] ;
							northeast_looping_base[j] 	<= northeast_address_base[j] ;
							northwest_looping_base[j] 	<= northwest_address_base[j] ;
							southeast_looping_base[j] 	<= southeast_address_base[j] ;
							southwest_looping_base[j] 	<= southwest_address_base[j] ;
							naught_looping_base[j] 		<= naught_address_base[j] ;
							barrier_looping_base[j] 	<= barrier_address_base[j] ;

							bounce_state[j] <= 8'd_21 ;
						end

						// Start filling up the pipe (looping bases at 0)
						if (bounce_state[j] == 8'd_21) begin
							
							// SET ALL M10K READ ADDRESSES
				  			north_read_address[j] 		<= (north_looping_base[j]) ;
				  			south_read_address[j] 		<= (south_looping_base[j]) ;
				  			east_read_address[j] 		<= (east_looping_base[j]) ;
				  			west_read_address[j] 		<= (9'd_511 - west_looping_base[j]) ;
				  			northeast_read_address[j] 	<= (northeast_looping_base[j]) ;
				  			northwest_read_address[j] 	<= (9'd_511 - northwest_looping_base[j]) ;
				  			southeast_read_address[j] 	<= (southeast_looping_base[j]) ;
				  			southwest_read_address[j] 	<= (9'd_511 - southwest_looping_base[j]) ;

				  			// CLEAR M10K WRITE ENABLES
				  			barrier_write_enable[j]		<= 1'b_0 ;
				  			north_write_enable[j]		<= 1'b_0 ;
				  			south_write_enable[j]		<= 1'b_0 ;
				  			east_write_enable[j]		<= 1'b_0 ;
				  			west_write_enable[j]		<= 1'b_0 ;
				  			northeast_write_enable[j]	<= 1'b_0 ;
				  			northwest_write_enable[j]	<= 1'b_0 ;
				  			southeast_write_enable[j]	<= 1'b_0 ;
				  			southwest_write_enable[j]	<= 1'b_0 ;

				  			// UPDATE LOOPING BASES
				  			north_looping_base[j] 		<= (north_looping_base[j] + 9'd_1) ;
				  			south_looping_base[j] 		<= (south_looping_base[j] + 9'd_1) ;
				  			east_looping_base[j] 		<= (east_looping_base[j] + 9'd_1) ;
				  			west_looping_base[j] 		<= (west_looping_base[j] + 9'd_1) ;
				  			northeast_looping_base[j] 	<= (northeast_looping_base[j] + 9'd_1) ;
				  			northwest_looping_base[j] 	<= (northwest_looping_base[j] + 9'd_1) ;
				  			southeast_looping_base[j] 	<= (southeast_looping_base[j] + 9'd_1) ;
				  			southwest_looping_base[j] 	<= (southwest_looping_base[j] + 9'd_1) ;

				  			// UNCONDITIONAL STATE TRANSITION
				  			bounce_state[j] <= 8'd_22 ;

						end

						// Continue filling up the pipe (looping bases now at 1)
						if (bounce_state[j] == 8'd_22) begin
							
							// SET ALL M10K READ ADDRESSES
				  			north_read_address[j] 		<= (north_looping_base[j]) ;
				  			south_read_address[j] 		<= (south_looping_base[j]) ;
				  			east_read_address[j] 		<= (east_looping_base[j]) ;
				  			west_read_address[j] 		<= (9'd_511 - west_looping_base[j]) ;
				  			northeast_read_address[j] 	<= (northeast_looping_base[j]) ;
				  			northwest_read_address[j] 	<= (9'd_511 - northwest_looping_base[j]) ;
				  			southeast_read_address[j] 	<= (southeast_looping_base[j]) ;
				  			southwest_read_address[j] 	<= (9'd_511 - southwest_looping_base[j]) ;

				  			// CLEAR M10K WRITE ENABLES
				  			barrier_write_enable[j]		<= 1'b_0 ;
				  			north_write_enable[j]		<= 1'b_0 ;
				  			south_write_enable[j]		<= 1'b_0 ;
				  			east_write_enable[j]		<= 1'b_0 ;
				  			west_write_enable[j]		<= 1'b_0 ;
				  			northeast_write_enable[j]	<= 1'b_0 ;
				  			northwest_write_enable[j]	<= 1'b_0 ;
				  			southeast_write_enable[j]	<= 1'b_0 ;
				  			southwest_write_enable[j]	<= 1'b_0 ;

				  			// UPDATE LOOPING BASES
				  			north_looping_base[j] 		<= (north_looping_base[j] + 9'd_1) ;
				  			south_looping_base[j] 		<= (south_looping_base[j] + 9'd_1) ;
				  			east_looping_base[j] 		<= (east_looping_base[j] + 9'd_1) ;
				  			west_looping_base[j] 		<= (west_looping_base[j] + 9'd_1) ;
				  			northeast_looping_base[j] 	<= (northeast_looping_base[j] + 9'd_1) ;
				  			northwest_looping_base[j] 	<= (northwest_looping_base[j] + 9'd_1) ;
				  			southeast_looping_base[j] 	<= (southeast_looping_base[j] + 9'd_1) ;
				  			southwest_looping_base[j] 	<= (southwest_looping_base[j] + 9'd_1) ;

				  			// UNCONDITIONAL STATE TRANSITION
				  			bounce_state[j] <= 8'd_23 ;

						end

						// Continue filling up the pipe (looping bases now at 2)
						if (bounce_state[j] == 8'd_23) begin

							// SET ALL M10K WRITE DATA
							north_data_in[j] 			<= north_data_out[(j+1) & 5'b_11111] ;
							south_data_in[j] 			<= south_data_out[(j+31) & 5'b_11111] ;
							east_data_in[j] 			<= east_data_out[j] ;
							northeast_data_in[j] 		<= northeast_data_out[(j+1) & 5'b_11111] ;
							southeast_data_in[j] 		<= southeast_data_out[(j+31) & 5'b_11111] ;
							west_data_in[j] 			<= west_data_out[j] ;
							northwest_data_in[j] 		<= northwest_data_out[(j+1) & 5'b_11111] ;
							southwest_data_in[j] 		<= southwest_data_out[(j+31) & 5'b_11111] ;

							// SET ALL M10K WRITE ADDRESSES
							north_write_address[j] 		<= (north_looping_base[j] + 9'd_510) ;
							south_write_address[j] 		<= (south_looping_base[j] + 9'd_510) ;
							east_write_address[j] 		<= (east_looping_base[j] + 9'd_511) ;
							northeast_write_address[j] 	<= (northeast_looping_base[j] + 9'd_511) ;
							southeast_write_address[j] 	<= (southeast_looping_base[j] + 9'd_511) ;
							west_write_address[j] 		<= ((9'd_511 - west_looping_base[j]) + 9'd_1) ;
							northwest_write_address[j] 	<= ((9'd_511 - northwest_looping_base[j]) + 9'd_1) ;
							southwest_write_address[j] 	<= ((9'd_511 - southwest_looping_base[j]) + 9'd_1) ;

							// SET M10K WRITE ENABLES
				  			north_write_enable[j]		<= 1'b_1 ;
				  			south_write_enable[j]		<= 1'b_1 ;
				  			east_write_enable[j]		<= 1'b_1 ;
				  			west_write_enable[j]		<= 1'b_1 ;
				  			northeast_write_enable[j]	<= 1'b_1 ;
				  			northwest_write_enable[j]	<= 1'b_1 ;
				  			southeast_write_enable[j]	<= 1'b_1 ;
				  			southwest_write_enable[j]	<= 1'b_1 ;
							
							// SET ALL M10K READ ADDRESSES
				  			north_read_address[j] 		<= (north_looping_base[j]) ;
				  			south_read_address[j] 		<= (south_looping_base[j]) ;
				  			east_read_address[j] 		<= (east_looping_base[j]) ;
				  			west_read_address[j] 		<= (9'd_511 - west_looping_base[j]) ;
				  			northeast_read_address[j] 	<= (northeast_looping_base[j]) ;
				  			northwest_read_address[j] 	<= (9'd_511 - northwest_looping_base[j]) ;
				  			southeast_read_address[j] 	<= (southeast_looping_base[j]) ;
				  			southwest_read_address[j] 	<= (9'd_511 - southwest_looping_base[j]) ;

				  			// UPDATE LOOPING BASES
				  			north_looping_base[j] 		<= (north_looping_base[j] + 9'd_1) ;
				  			south_looping_base[j] 		<= (south_looping_base[j] + 9'd_1) ;
				  			east_looping_base[j] 		<= (east_looping_base[j] + 9'd_1) ;
				  			west_looping_base[j] 		<= (west_looping_base[j] + 9'd_1) ;
				  			northeast_looping_base[j] 	<= (northeast_looping_base[j] + 9'd_1) ;
				  			northwest_looping_base[j] 	<= (northwest_looping_base[j] + 9'd_1) ;
				  			southeast_looping_base[j] 	<= (southeast_looping_base[j] + 9'd_1) ;
				  			southwest_looping_base[j] 	<= (southwest_looping_base[j] + 9'd_1) ;

				  			// CONDITIONAL STATE TRANSITION
				  			bounce_state[j] <= (north_looping_base[j] == 9'd_1) ? 8'd_0 : 8'd_23 ;

						end

						// BOUNCING
						// Initialize all addresses
						if (bounce_state[j] == 8'd_0) begin
							north_looping_base[j] 		<= north_address_base[j] ;
							south_looping_base[j] 		<= south_address_base[j] ;
							east_looping_base[j] 		<= east_address_base[j] ;
							west_looping_base[j] 		<= west_address_base[j] ;
							northeast_looping_base[j] 	<= northeast_address_base[j] ;
							northwest_looping_base[j] 	<= northwest_address_base[j] ;
							southeast_looping_base[j] 	<= southeast_address_base[j] ;
							southwest_looping_base[j] 	<= southwest_address_base[j] ;
							naught_looping_base[j] 		<= naught_address_base[j] ;
							barrier_looping_base[j] 	<= barrier_address_base[j] ;

							// CLEAR M10K WRITE ENABLES
				  			barrier_write_enable[j]		<= 1'b_0 ;
				  			north_write_enable[j]		<= 1'b_0 ;
				  			south_write_enable[j]		<= 1'b_0 ;
				  			east_write_enable[j]		<= 1'b_0 ;
				  			west_write_enable[j]		<= 1'b_0 ;
				  			northeast_write_enable[j]	<= 1'b_0 ;
				  			northwest_write_enable[j]	<= 1'b_0 ;
				  			southeast_write_enable[j]	<= 1'b_0 ;
				  			southwest_write_enable[j]	<= 1'b_0 ;

							bounce_state[j] <= 8'd_1 ;
						end

						// Start filling up the pipe, we read from the SAME relative cells, write to different cells
						if (bounce_state[j] == 8'd_1) begin

							// SET ALL M10K READ ADDRESSES
				  			barrier_read_address[j] 	<= (barrier_looping_base[j]) ;
				  			north_read_address[j] 		<= (north_looping_base[j]) ;
				  			south_read_address[j] 		<= (south_looping_base[j]) ;
				  			east_read_address[j] 		<= (east_looping_base[j]) ;
				  			west_read_address[j] 		<= (west_looping_base[j]) ;
				  			northeast_read_address[j] 	<= (northeast_looping_base[j]) ;
				  			northwest_read_address[j] 	<= (northwest_looping_base[j]) ;
				  			southeast_read_address[j] 	<= (southeast_looping_base[j]) ;
				  			southwest_read_address[j] 	<= (southwest_looping_base[j]) ;
				  			naught_read_address[j] 		<= (naught_looping_base[j]) ;

				  			// CLEAR M10K WRITE ENABLES
				  			barrier_write_enable[j]		<= 1'b_0 ;
				  			north_write_enable[j]		<= 1'b_0 ;
				  			south_write_enable[j]		<= 1'b_0 ;
				  			east_write_enable[j]		<= 1'b_0 ;
				  			west_write_enable[j]		<= 1'b_0 ;
				  			northeast_write_enable[j]	<= 1'b_0 ;
				  			northwest_write_enable[j]	<= 1'b_0 ;
				  			southeast_write_enable[j]	<= 1'b_0 ;
				  			southwest_write_enable[j]	<= 1'b_0 ;
				  			naught_write_enable[j]		<= 1'b_0 ;

				  			// UPDATE LOOPING BASES
				  			barrier_looping_base[j] 	<= (barrier_looping_base[j] + 9'd_1) ;
				  			north_looping_base[j] 		<= (north_looping_base[j] + 9'd_1) ;
				  			south_looping_base[j] 		<= (south_looping_base[j] + 9'd_1) ;
				  			east_looping_base[j] 		<= (east_looping_base[j] + 9'd_1) ;
				  			west_looping_base[j] 		<= (west_looping_base[j] + 9'd_1) ;
				  			northeast_looping_base[j] 	<= (northeast_looping_base[j] + 9'd_1) ;
				  			northwest_looping_base[j] 	<= (northwest_looping_base[j] + 9'd_1) ;
				  			southeast_looping_base[j] 	<= (southeast_looping_base[j] + 9'd_1) ;
				  			southwest_looping_base[j] 	<= (southwest_looping_base[j] + 9'd_1) ;
				  			naught_looping_base[j] 		<= (naught_looping_base[j] + 9'd_1) ;

							// TRANSITION STATE
							bounce_state[j]	<= 8'd_2 ;

						end

						// Continue filling up the pipe (assumes 2-cycle delay on M10k read)
						if (bounce_state[j] == 8'd_2) begin

							// SET ALL M10K READ ADDRESSES
				  			barrier_read_address[j] 	<= (barrier_looping_base[j]) ;
				  			north_read_address[j] 		<= (north_looping_base[j]) ;
				  			south_read_address[j] 		<= (south_looping_base[j]) ;
				  			east_read_address[j] 		<= (east_looping_base[j]) ;
				  			west_read_address[j] 		<= (west_looping_base[j]) ;
				  			northeast_read_address[j] 	<= (northeast_looping_base[j]) ;
				  			northwest_read_address[j] 	<= (northwest_looping_base[j]) ;
				  			southeast_read_address[j] 	<= (southeast_looping_base[j]) ;
				  			southwest_read_address[j] 	<= (southwest_looping_base[j]) ;
				  			naught_read_address[j] 		<= (naught_looping_base[j]) ;

				  			// CLEAR M10K WRITE ENABLES
				  			barrier_write_enable[j]		<= 1'b_0 ;
				  			north_write_enable[j]		<= 1'b_0 ;
				  			south_write_enable[j]		<= 1'b_0 ;
				  			east_write_enable[j]		<= 1'b_0 ;
				  			west_write_enable[j]		<= 1'b_0 ;
				  			northeast_write_enable[j]	<= 1'b_0 ;
				  			northwest_write_enable[j]	<= 1'b_0 ;
				  			southeast_write_enable[j]	<= 1'b_0 ;
				  			southwest_write_enable[j]	<= 1'b_0 ;
				  			naught_write_enable[j]		<= 1'b_0 ;

				  			// UPDATE LOOPING BASES
				  			barrier_looping_base[j] 	<= (barrier_looping_base[j] + 9'd_1) ;
				  			north_looping_base[j] 		<= (north_looping_base[j] + 9'd_1) ;
				  			south_looping_base[j] 		<= (south_looping_base[j] + 9'd_1) ;
				  			east_looping_base[j] 		<= (east_looping_base[j] + 9'd_1) ;
				  			west_looping_base[j] 		<= (west_looping_base[j] + 9'd_1) ;
				  			northeast_looping_base[j] 	<= (northeast_looping_base[j] + 9'd_1) ;
				  			northwest_looping_base[j] 	<= (northwest_looping_base[j] + 9'd_1) ;
				  			southeast_looping_base[j] 	<= (southeast_looping_base[j] + 9'd_1) ;
				  			southwest_looping_base[j] 	<= (southwest_looping_base[j] + 9'd_1) ;
				  			naught_looping_base[j] 		<= (naught_looping_base[j] + 9'd_1) ;

							// TRANSITION STATE
							bounce_state[j]	<= 8'd_3 ;
						end

						// If you find a barrier, push densities back from whence they came.
						// Doesn't clear barrier cell in this state.
						if (bounce_state[j] == 8'd_3) begin

							// SET M10K WRITE DATA
							north_data_in[j] 		<= south_data_out[(j+1) & 5'b_11111] ;
							south_data_in[j] 		<= north_data_out[(j+31) & 5'b_11111] ;
							east_data_in[j] 		<= west_data_out[j] ;
							west_data_in[j] 		<= east_data_out[j] ;
							northeast_data_in[j] 	<= southwest_data_out[(j+1) & 5'b_11111] ;
							northwest_data_in[j] 	<= southeast_data_out[(j+1) & 5'b_11111] ;
							southeast_data_in[j] 	<= northwest_data_out[(j+31) & 5'b_11111] ;
							southwest_data_in[j] 	<= northeast_data_out[(j+31) & 5'b_11111] ;

							// SET M10K WRITE ADDRESSES
							north_write_address[j] 		<= (north_looping_base[j] + 9'd_510) ;
							south_write_address[j] 		<= (south_looping_base[j] + 9'd_510) ;
							east_write_address[j] 		<= (east_looping_base[j] + 9'd_511) ;
							west_write_address[j] 		<= (west_looping_base[j] + 9'd_509) ;
							northeast_write_address[j] 	<= (northeast_looping_base[j] + 9'd_511) ;
							northwest_write_address[j] 	<= (northwest_looping_base[j] + 9'd_509) ;
							southeast_write_address[j] 	<= (southeast_looping_base[j] + 9'd_511) ;
							southwest_write_address[j] 	<= (southwest_looping_base[j] + 9'd_509) ;

							// SET WRITE ENABLE IF A BARRIER WAS PRESENT
				  			barrier_write_enable[j]		<= 1'b_0 ;
				  			north_write_enable[j]		<= (barrier_data_out[(j+1) & 5'b_11111]!=20'd_0) ? 1'b_1 : 1'b_0 ;
				  			south_write_enable[j]		<= (barrier_data_out[(j+31) & 5'b_11111]!=20'd_0) ? 1'b_1 : 1'b_0 ;
				  			east_write_enable[j]		<= (barrier_data_out[j]!=20'd_0) ? 1'b_1 : 1'b_0 ;
				  			west_write_enable[j]		<= (barrier_data_out[j]!=20'd_0) ? 1'b_1 : 1'b_0 ;
				  			northeast_write_enable[j]	<= (barrier_data_out[(j+1) & 5'b_11111]!=20'd_0) ? 1'b_1 : 1'b_0 ;
				  			northwest_write_enable[j]	<= (barrier_data_out[(j+1) & 5'b_11111]!=20'd_0) ? 1'b_1 : 1'b_0 ;
				  			southeast_write_enable[j]	<= (barrier_data_out[(j+31) & 5'b_11111]!=20'd_0) ? 1'b_1 : 1'b_0 ;
				  			southwest_write_enable[j]	<= (barrier_data_out[(j+31) & 5'b_11111]!=20'd_0) ? 1'b_1 : 1'b_0 ;
				  			naught_write_enable[j]		<= 1'b_0 ;

							// UPDATE ALL M10K READ ADDRESSES
				  			barrier_read_address[j] 	<= (barrier_looping_base[j]) ;
				  			north_read_address[j] 		<= (north_looping_base[j]) ;
				  			south_read_address[j] 		<= (south_looping_base[j]) ;
				  			east_read_address[j] 		<= (east_looping_base[j]) ;
				  			west_read_address[j] 		<= (west_looping_base[j]) ;
				  			northeast_read_address[j] 	<= (northeast_looping_base[j]) ;
				  			northwest_read_address[j] 	<= (northwest_looping_base[j]) ;
				  			southeast_read_address[j] 	<= (southeast_looping_base[j]) ;
				  			southwest_read_address[j] 	<= (southwest_looping_base[j]) ;
				  			naught_read_address[j] 		<= (naught_looping_base[j]) ;

				  			// UPDATE LOOPING BASES
				  			barrier_looping_base[j] 	<= (barrier_looping_base[j] + 9'd_1) ;
				  			north_looping_base[j] 		<= (north_looping_base[j] + 9'd_1) ;
				  			south_looping_base[j] 		<= (south_looping_base[j] + 9'd_1) ;
				  			east_looping_base[j] 		<= (east_looping_base[j] + 9'd_1) ;
				  			west_looping_base[j] 		<= (west_looping_base[j] + 9'd_1) ;
				  			northeast_looping_base[j] 	<= (northeast_looping_base[j] + 9'd_1) ;
				  			northwest_looping_base[j] 	<= (northwest_looping_base[j] + 9'd_1) ;
				  			southeast_looping_base[j] 	<= (southeast_looping_base[j] + 9'd_1) ;
				  			southwest_looping_base[j] 	<= (southwest_looping_base[j] + 9'd_1) ;
				  			naught_looping_base[j] 		<= (naught_looping_base[j] + 9'd_1) ;


				  			// CONDITIONAL STATE TRANSITION
				  			bounce_state[j] <= (barrier_looping_base[j] == (barrier_address_base[j] + 9'd_1)) ? 8'd_4 : 8'd_3 ;
						end

						// Clear cells with barriers (now writing same cells which we read)
						if (bounce_state[j] == 8'd_4) begin

							// SET M10K WRITE DATA
							north_data_in[j] 		<= 20'd_0 ;
							south_data_in[j] 		<= 20'd_0 ;
							east_data_in[j] 		<= 20'd_0 ;
							west_data_in[j] 		<= 20'd_0 ;
							northeast_data_in[j] 	<= 20'd_0 ;
							northwest_data_in[j] 	<= 20'd_0 ;
							southeast_data_in[j] 	<= 20'd_0 ;
							southwest_data_in[j] 	<= 20'd_0 ;
							naught_data_in[j] 		<= 20'd_0 ;

							// SET M10K WRITE ADDRESSES
							north_write_address[j] 		<= (north_looping_base[j] + 9'd_510) ;
							south_write_address[j] 		<= (south_looping_base[j] + 9'd_510) ;
							east_write_address[j] 		<= (east_looping_base[j] + 9'd_510) ;
							west_write_address[j] 		<= (west_looping_base[j] + 9'd_510) ;
							northeast_write_address[j] 	<= (northeast_looping_base[j] + 9'd_510) ;
							northwest_write_address[j] 	<= (northwest_looping_base[j] + 9'd_510) ;
							southeast_write_address[j] 	<= (southeast_looping_base[j] + 9'd_510) ;
							southwest_write_address[j] 	<= (southwest_looping_base[j] + 9'd_510) ;
							naught_write_address[j] 	<= (naught_looping_base[j] + 9'd_510) ;

							// SET WRITE ENABLE IF A BARRIER WAS PRESENT
				  			barrier_write_enable[j]		<= 1'b_0 ;
				  			north_write_enable[j]		<= (barrier_data_out[j]!=20'd_0) ? 1'b_1 : 1'b_0 ;
				  			south_write_enable[j]		<= (barrier_data_out[j]!=20'd_0) ? 1'b_1 : 1'b_0 ;
				  			east_write_enable[j]		<= (barrier_data_out[j]!=20'd_0) ? 1'b_1 : 1'b_0 ;
				  			west_write_enable[j]		<= (barrier_data_out[j]!=20'd_0) ? 1'b_1 : 1'b_0 ;
				  			northeast_write_enable[j]	<= (barrier_data_out[j]!=20'd_0) ? 1'b_1 : 1'b_0 ;
				  			northwest_write_enable[j]	<= (barrier_data_out[j]!=20'd_0) ? 1'b_1 : 1'b_0 ;
				  			southeast_write_enable[j]	<= (barrier_data_out[j]!=20'd_0) ? 1'b_1 : 1'b_0 ;
				  			southwest_write_enable[j]	<= (barrier_data_out[j]!=20'd_0) ? 1'b_1 : 1'b_0 ;
				  			naught_write_enable[j]		<= (barrier_data_out[j]!=20'd_0) ? 1'b_1 : 1'b_0 ;

							// UPDATE ALL M10K READ ADDRESSES
				  			barrier_read_address[j] 	<= (barrier_looping_base[j]) ;
				  			north_read_address[j] 		<= (north_looping_base[j]) ;
				  			south_read_address[j] 		<= (south_looping_base[j]) ;
				  			east_read_address[j] 		<= (east_looping_base[j]) ;
				  			west_read_address[j] 		<= (west_looping_base[j]) ;
				  			northeast_read_address[j] 	<= (northeast_looping_base[j]) ;
				  			northwest_read_address[j] 	<= (northwest_looping_base[j]) ;
				  			southeast_read_address[j] 	<= (southeast_looping_base[j]) ;
				  			southwest_read_address[j] 	<= (southwest_looping_base[j]) ;
				  			naught_read_address[j] 		<= (naught_looping_base[j]) ;

				  			// UPDATE LOOPING BASES
				  			barrier_looping_base[j] 	<= (barrier_looping_base[j] + 9'd_1) ;
				  			north_looping_base[j] 		<= (north_looping_base[j] + 9'd_1) ;
				  			south_looping_base[j] 		<= (south_looping_base[j] + 9'd_1) ;
				  			east_looping_base[j] 		<= (east_looping_base[j] + 9'd_1) ;
				  			west_looping_base[j] 		<= (west_looping_base[j] + 9'd_1) ;
				  			northeast_looping_base[j] 	<= (northeast_looping_base[j] + 9'd_1) ;
				  			northwest_looping_base[j] 	<= (northwest_looping_base[j] + 9'd_1) ;
				  			southeast_looping_base[j] 	<= (southeast_looping_base[j] + 9'd_1) ;
				  			southwest_looping_base[j] 	<= (southwest_looping_base[j] + 9'd_1) ;
				  			naught_looping_base[j] 		<= (naught_looping_base[j] + 9'd_1) ;

				  			// CONDITIONAL STATE TRANSITION
				  			bounce_state[j] <= (barrier_looping_base[j] == (barrier_address_base[j] + 9'd_1)) ? 8'd_5 : 8'd_4 ;
							
						end

						// COLLISION (looping base should be +2 when we enter this state)
						if (bounce_state[j] == 8'd_5) begin

							// CLEAR ALL WRITE ENABLES
				  			barrier_write_enable[j]		<= 1'b_0 ;
				  			north_write_enable[j]		<= 1'b_0 ;
				  			south_write_enable[j]		<= 1'b_0 ;
				  			east_write_enable[j]		<= 1'b_0 ;
				  			west_write_enable[j]		<= 1'b_0 ;
				  			northeast_write_enable[j]	<= 1'b_0 ;
				  			northwest_write_enable[j]	<= 1'b_0 ;
				  			southeast_write_enable[j]	<= 1'b_0 ;
				  			southwest_write_enable[j]	<= 1'b_0 ;
				  			naught_write_enable[j]		<= 1'b_0 ;
				  			speed_write_enable[j] 		<= 1'b_0 ;

				  			// REGISTER MICRODENSITIES
				  			n0[j] 	<= 	naught_data_out[j] ;
				  			nN[j] 	<= 	north_data_out[j] ;
				  			nS[j] 	<= 	south_data_out[j] ;
				  			nE[j] 	<= 	east_data_out[j] ;
				  			nW[j] 	<= 	west_data_out[j] ;
				  			nNE[j] 	<= 	northeast_data_out[j] ;
				  			nNW[j] 	<= 	northwest_data_out[j] ;
				  			nSE[j] 	<= 	southeast_data_out[j] ;
				  			nSW[j] 	<= 	southwest_data_out[j] ;


				  			bounce_done[j] <= 1'b_1 ;

				  			// UNCONDITIONAL STATE TRANSITION
				  			bounce_state[j] <= 8'd_6 ;
							
						end

						// DO I NEED AN EXTRA STATE?
						if (bounce_state[j] == 8'd_6) begin
							
							// REGISTER RHO (is this addition my critical path? add a state?)
							rho[j] <= rho_wire[j] ;
							// REGISTER RHO-1
							rho_m_1[j] 	<= (rho_wire[j] - fixone) ;
							// SET INPUTS FOR 1/9 RHO
							mult_1_in_1[j] <= one9th ;
							mult_1_in_2[j] <= rho_wire[j] ;
							// SET INPUTS FOR 1/36 RHO
							mult_2_in_1[j] <= one36th ;
							mult_2_in_2[j] <= rho_wire[j] ;
							// UNCONDITIONAL STATE TRANSITION
							bounce_state[j] <= 8'd_7 ;

						end

						if (bounce_state[j] == 8'd_7) begin
							
							// REGISTER 1/9 RHO
							one9th_rho[j] <= mult_1_out[j] ;
							// REGISTER 1/36 RHO
							one36th_rho[j] <= mult_2_out[j] ;
							// SET INPUTS FOR RHO_INV
							mult_1_in_1[j] <= rho_m_1[j] ;
							mult_1_in_2[j] <= rho_m_1[j] ;
							// UNCONDITIONAL STATE TRANSITION
							bounce_state[j] <= 8'd_8 ;

						end

						if (bounce_state[j] == 8'd_8) begin
							
							// REGISTER RHO_INV (TAYLOR SERIES APPROX.)
							rho_inv[j] <= (fixone - rho_m_1[j] + mult_1_out[j]) ;
							// SET INPUTS FOR Ux
							mult_1_in_1[j] <= ux_num[j] ;
							mult_1_in_2[j] <= (fixone - rho_m_1[j] + mult_1_out[j]) ;
							// SET INPUTS FOR Uy
							mult_2_in_1[j] <= uy_num[j] ;
							mult_2_in_2[j] <= (fixone - rho_m_1[j] + mult_1_out[j]) ;
							// UNCONDITIONAL STATE TRANSITION
							bounce_state[j] <= 8'd_9 ;
							
						end

						if (bounce_state[j] == 8'd_9) begin
							
							// REGISTER Ux, AUTOCOMPUTES 3Ux
							ux[j] <= mult_1_out[j] ;
							// REGISTER Uy, AUTOCOMPUTES 3Uy
							uy[j] <= mult_2_out[j] ;
							// SET INPUTS FOR Ux^2
							mult_1_in_1[j] <= mult_1_out[j] ;
							mult_1_in_2[j] <= mult_1_out[j] ;
							// SET INPUTS FOR Uy^2
							mult_2_in_1[j] <= mult_2_out[j] ;
							mult_2_in_2[j] <= mult_2_out[j] ;
							// UNCONDITIONAL STATE TRANSITION
							bounce_state[j] <= 8'd_10 ;

						end

						if (bounce_state[j] == 8'd_10) begin
							
							// REGISTER Ux^2
							vx2[j] <= mult_1_out[j] ;
							// REGISTER Uy^2
							vy2[j] <= mult_2_out[j] ;
							// SET INPUTS FOR 2UxUy
							mult_1_in_1[j] <= (ux[j] << 1) ;
							mult_1_in_2[j] <=  uy[j] ;
							// UNCONDITIONAL STATE TRANSITION
							bounce_state[j] <= 8'd_11 ;

						end

						if (bounce_state[j] == 8'd_11) begin
							
							// REGISTERS 2UxUy
							vxvy2[j] <= mult_1_out[j] ;
							// COMBINATIONAL VALUES AUTOCOMPUTED
							// UNCONDITIONAL STATE TRANSITION
							bounce_state[j] <= 8'd_12 ;

						end

						// IS AN EXTRA STATE NEEDED HERE? LONG CRITICAL PATH?
						if (bounce_state[j] == 8'd_12) begin
							
							// SET INPUTS FOR nEm
							mult_1_in_1[j] <= (fixone + vx3[j] + vx2_4_5[j] - v215[j]) ;
							mult_1_in_2[j] <= (one9th_rho[j]) ;
							// SET INPUTS FOR nWm
							mult_2_in_1[j] <= (fixone - vx3[j] + vx2_4_5[j] - v215[j]) ;
							mult_2_in_2[j] <= (one9th_rho[j]) ;
							// UNCONDITIONAL STATE TRANSITION
							bounce_state[j] <= 8'd_13 ;

						end

						if (bounce_state[j] == 8'd_13) begin
							
							// REGISTER nEm
							nEm[j] <= mult_1_out[j] ;
							// REGISTER nWm
							nWm[j] <= mult_2_out[j] ;
							// SET INPUTS FOR nNm
							mult_1_in_1[j] <= (fixone + vy3[j] + vy2_4_5[j] - v215[j]) ;
							mult_1_in_2[j] <= (one9th_rho[j]) ;
							// SET INPUTS FOR nSm
							mult_2_in_1[j] <= (fixone - vy3[j] + vy2_4_5[j] - v215[j]) ;
							mult_2_in_2[j] <= (one9th_rho[j]) ;
							// UNCONDITIONAL STATE TRANSITION
							bounce_state[j] <= 8'd_14 ;

						end

						if (bounce_state[j] == 8'd_14) begin
							
							// REGISTER nNm
							nNm[j] <= mult_1_out[j] ;
							// REGISTER nSm
							nSm[j] <= mult_2_out[j] ;
							// SET INPUTS FOR nNEm
							mult_1_in_1[j] <= (fixone + vx3[j] + vy3[j] + v2_p_2vxvy_4_5[j] - v215[j]) ;
							mult_1_in_2[j] <= (one36th_rho[j]) ;
							// SET INPUTS FOR nNWm
							mult_2_in_1[j] <= (fixone - vx3[j] + vy3[j] + v2_m_2vxvy_4_5[j] - v215[j]) ;
							mult_2_in_2[j] <= (one36th_rho[j]) ;
							// UNCONDITIONAL STATE TRANSITION
							bounce_state[j] <= 8'd_15 ;

						end

						if (bounce_state[j] == 8'd_15) begin
							
							// REGISTER nNEm
							nNEm[j] <= mult_1_out[j] ;
							// REGISTER nNWm
							nNWm[j] <= mult_2_out[j] ;
							// SET INPUTS FOR nSEm
							mult_1_in_1[j] <= (fixone + vx3[j] - vy3[j] + v2_m_2vxvy_4_5[j] - v215[j]) ;
							mult_1_in_2[j] <= (one36th_rho[j]) ;
							// SET INPUTS FOR nSWm
							mult_2_in_1[j] <= (fixone - vx3[j] - vy3[j] + v2_p_2vxvy_4_5[j] - v215[j]) ;
							mult_2_in_2[j] <= (one36th_rho[j]) ;
							// UNCONDITIONAL STATE TRANSITION
							bounce_state[j] <= 8'd_16 ;

						end

						if (bounce_state[j] == 8'd_16) begin
							
							// REGISTER nSEm
							nSEm[j] <= mult_1_out[j] ;
							// REGISTER nSWm
							nSWm[j] <= mult_2_out[j] ;
							// AUTOCOMPUTE UPDATE MICRODENSITIES
							// UNCONDITIONAL STATE TRANSITION
							bounce_state[j] <= 8'd_17 ;

						end

						// DO I NEED AN EXTRA STATE HERE?
						if (bounce_state[j] == 8'd_17) begin

							// SET M10K WRITE DATA (ENFORCE BOUNDARY CONDITIONS)
							north_data_in[j] 		<= ((j==0) ? north_init : ((j==num_rows) ? north_init : ((north_looping_base[j]==9'd_2) ? north_init : nN_new[j]))) ;
							south_data_in[j] 		<= ((j==0) ? south_init : ((j==num_rows) ? south_init : ((north_looping_base[j]==9'd_2) ? south_init : nS_new[j]))) ;
							east_data_in[j] 		<= ((j==0) ? east_init : ((j==num_rows) ? east_init : ((north_looping_base[j]==9'd_2) ? east_init : nE_new[j]))) ;
							west_data_in[j] 		<= ((j==0) ? west_init : ((j==num_rows) ? west_init : ((north_looping_base[j]==9'd_2) ? west_init : nW_new[j]))) ;
							northeast_data_in[j] 	<= ((j==0) ? northeast_init : ((j==num_rows) ? northeast_init : ((north_looping_base[j]==9'd_2) ? northeast_init : nNE_new[j]))) ;
							northwest_data_in[j] 	<= ((j==0) ? northwest_init : ((j==num_rows) ? northwest_init : ((north_looping_base[j]==9'd_2) ? northwest_init : nNW_new[j]))) ;
							southeast_data_in[j] 	<= ((j==0) ? southeast_init : ((j==num_rows) ? southeast_init : ((north_looping_base[j]==9'd_2) ? southeast_init : nSE_new[j]))) ;
							southwest_data_in[j] 	<= ((j==0) ? southwest_init : ((j==num_rows) ? southwest_init : ((north_looping_base[j]==9'd_2) ? southwest_init : nSW_new[j]))) ;
							naught_data_in[j] 		<= ((j==0) ? naught_init : ((j==num_rows) ? naught_init : ((north_looping_base[j]==9'd_2) ? naught_init : n0_new[j]))) ;
							speed_data_in[j] 		<= ((j==0) ? 20'd_0 : ((j==num_rows) ? 20'd_0 : ((north_looping_base[j]==9'd_2) ? 20'd_0 : v2[j]))) ;

							// SET M10K WRITE ADDRESSES (2 behind)
							north_write_address[j] 		<= (north_looping_base[j] + 9'd_510) ;
							south_write_address[j] 		<= (south_looping_base[j] + 9'd_510) ;
							east_write_address[j] 		<= (east_looping_base[j] + 9'd_510) ;
							west_write_address[j] 		<= (west_looping_base[j] + 9'd_510) ;
							northeast_write_address[j] 	<= (northeast_looping_base[j] + 9'd_510) ;
							northwest_write_address[j] 	<= (northwest_looping_base[j] + 9'd_510) ;
							southeast_write_address[j] 	<= (southeast_looping_base[j] + 9'd_510) ;
							southwest_write_address[j] 	<= (southwest_looping_base[j] + 9'd_510) ;
							naught_write_address[j] 	<= (naught_looping_base[j] + 9'd_510) ;
							speed_write_address[j] 		<= (naught_looping_base[j] + 9'd_510) ;

							// SET M10K WRITE ENABLE
				  			north_write_enable[j]		<= 1'b_1 ;
				  			south_write_enable[j]		<= 1'b_1 ;
				  			east_write_enable[j]		<= 1'b_1 ;
				  			west_write_enable[j]		<= 1'b_1 ;
				  			northeast_write_enable[j]	<= 1'b_1 ;
				  			northwest_write_enable[j]	<= 1'b_1 ;
				  			southeast_write_enable[j]	<= 1'b_1 ;
				  			southwest_write_enable[j]	<= 1'b_1 ;
				  			naught_write_enable[j]		<= 1'b_1 ;
				  			speed_write_enable[j] 		<= 1'b_1 ;

							// UPDATE LOOPING BASES
				  			barrier_looping_base[j] 	<= (barrier_looping_base[j] + 9'd_1) ;
				  			north_looping_base[j] 		<= (north_looping_base[j] + 9'd_1) ;
				  			south_looping_base[j] 		<= (south_looping_base[j] + 9'd_1) ;
				  			east_looping_base[j] 		<= (east_looping_base[j] + 9'd_1) ;
				  			west_looping_base[j] 		<= (west_looping_base[j] + 9'd_1) ;
				  			northeast_looping_base[j] 	<= (northeast_looping_base[j] + 9'd_1) ;
				  			northwest_looping_base[j] 	<= (northwest_looping_base[j] + 9'd_1) ;
				  			southeast_looping_base[j] 	<= (southeast_looping_base[j] + 9'd_1) ;
				  			southwest_looping_base[j] 	<= (southwest_looping_base[j] + 9'd_1) ;
				  			naught_looping_base[j] 		<= (naught_looping_base[j] + 9'd_1) ;

							// UPDATE READ ADDRESS (1 behind)
							north_read_address[j] 		<= (north_looping_base[j] + 9'd_511) ;
				  			south_read_address[j] 		<= (south_looping_base[j] + 9'd_511) ;
				  			east_read_address[j] 		<= (east_looping_base[j] + 9'd_511) ;
				  			west_read_address[j] 		<= (west_looping_base[j] + 9'd_511) ;
				  			northeast_read_address[j] 	<= (northeast_looping_base[j] + 9'd_511) ;
				  			northwest_read_address[j] 	<= (northwest_looping_base[j] + 9'd_511) ;
				  			southeast_read_address[j] 	<= (southeast_looping_base[j] + 9'd_511) ;
				  			southwest_read_address[j] 	<= (southwest_looping_base[j] + 9'd_511) ;
				  			naught_read_address[j] 		<= (naught_looping_base[j] + 9'd_511) ;
							
							// UNCONDITIONAL STATE TRANSITION
							bounce_state[j] <= 8'd_18 ;

						end

						if (bounce_state[j] == 8'd_18) begin
							
							// CLEAR WRITE ENABLES
							north_write_enable[j]		<= 1'b_0 ;
				  			south_write_enable[j]		<= 1'b_0 ;
				  			east_write_enable[j]		<= 1'b_0 ;
				  			west_write_enable[j]		<= 1'b_0 ;
				  			northeast_write_enable[j]	<= 1'b_0 ;
				  			northwest_write_enable[j]	<= 1'b_0 ;
				  			southeast_write_enable[j]	<= 1'b_0 ;
				  			southwest_write_enable[j]	<= 1'b_0 ;
				  			naught_write_enable[j]		<= 1'b_0 ;
				  			speed_write_enable[j] 		<= 1'b_0 ;

				  			// CONDITIONAL STATE TRANSITION (TERMINATE WHEN LOOPING BASE IS 1)
				  			bounce_state[j] <= (naught_looping_base[j]==9'd_1) ? 8'd_20: 8'd_5 ;
						end
					end
				end
			end
	endgenerate
endmodule

//////////////////////////////////////////////////
//// signed mult of 3.17 format 2'comp////////////
//////////////////////////////////////////////////

module signed_mult (out, a, b);
	output 	signed  [19:0]	out;
	input 	signed	[19:0] 	a;
	input 	signed	[19:0] 	b;
	// intermediate full bit length
	wire 	signed	[39:0]	mult_out;
	assign mult_out = a * b;
	// select bits
	assign out = {mult_out[39], mult_out[35:17]};
endmodule
//////////////////////////////////////////////////



module M10K_512_18( 
    output reg [19:0] q,
    input [19:0] d,
    input [8:0] write_address, read_address,
    input we, clk
);
	 // force M10K ram style
    reg [19:0] mem [511:0]  /* synthesis ramstyle = "no_rw_check, M10K" */;
	 
    always @ (posedge clk) begin
        if (we) begin
            mem[write_address] <= d;
        end
        // read_address_reg <= read_address ;
        q <= mem[read_address]; // q doesn't get d in this clock cycle
    end
endmodule