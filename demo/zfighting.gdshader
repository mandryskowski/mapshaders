shader_type spatial;

render_mode depth_draw_always;

void vertex() {
    VERTEX.z += 0.001;

}

//void light() {
	// Called for every pixel for every light affecting the material.
	// Uncomment to replace the default light processing function with this one.
//}
