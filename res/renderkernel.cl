__kernel void matMult(uint3 inputdim,
					  uint outputres,
					  __read_only image3d_t A,
                      __global char *C//,
							 ) {
	const uint x = get_global_id(0);//spalte
	const uint y = get_global_id(1);//zeile
	// the traversal loop,
	// termination when the sampling position is outside volume boundarys
	// another termination condition for early ray termination is added
	const float3 ray_entry_position = (float3)(x*inputdim[0]/(float)outputres, y*inputdim[1]/(float)outputres,0);//map to 0-1
	const float3 camera_location = (float3)(ray_entry_position.x, ray_entry_position.y,-1);//map to 0-1
	const float sampling_distance = 1;
	float3 ray_increment = normalize(ray_entry_position - camera_location) * sampling_distance;
	float4 sampling_pos = float4(0.0);
	float4 max_bounds = float4((float)inputdim[0],(float)inputdim[1],(float)inputdim[2],1.0);
	sampling_pos.xyz = ray_entry_position+ray_increment;
	float dst = 0.0;
	float trsp = 1;
	float brightnes = 1;
	const sampler_t sampler = CLK_NORMALIZED_COORDS_FALSE | CLK_ADDRESS_REPEAT |
	CLK_FILTER_LINEAR;
	while (all(sampling_pos >= 0) && all(sampling_pos < max_bounds)) {
		// get sample
		bool s = bool(read_imagei(A, sampler, sampling_pos).a);
		
		//early termination
		if (trsp <= 0.1 || dst>1.0){
			dst = min((float)1.0,dst);
			break;
		}

		#if ENABLE_LIGHTNING == 1 // Add Shading
			color *= get_shading(sampling_pos, ray_increment);
		#endif
		
		#if ENABLE_OPACITY_CORRECTION == 1 // Opacity Correction
			color.a = 1 - pow(1.0 - color.a, sampling_distance/ sampling_distance_ref*300 );
		#endif
		
		//front-to-back
		// accumulate color
		if (s==1){
			dst += 0.01 * trsp;
			// update opacity
			trsp *= 0.975;
		}
		
		// increment the ray sampling position
		sampling_pos.xyz += ray_increment;
	}
	C[y*outputres+x] = dst*255;
}
