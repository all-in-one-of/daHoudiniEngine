@vertexShader

uniform vec2 unif_TextureTiling;
varying vec2 var_TexCoord;

///////////////////////////////////////////////////////////////////////////////////////////////////
void setupSurfaceData(vec4 eyeSpacePosition)
{
    // bump
	// var_TexCoord = gl_MultiTexCoord0.xy * unif_TextureTiling;
    // textured
	var_TexCoord = gl_MultiTexCoord0.xy; //* unif_TextureTiling;
}