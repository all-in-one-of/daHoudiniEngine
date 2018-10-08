@surfaceShader
// @ envmap // used for environment map..

// Houdini Engine Shader
// The above surfaceShader token is used by cyclops

// The diffuse texture
uniform sampler2D unif_DiffuseMap;
uniform sampler2D unif_NormalMap; // for bump mapping
varying vec2 var_TexCoord;

// colour stuff
uniform float unif_Shininess;
uniform float unif_Gloss;

varying vec3 var_Normal;

///////////////////////////////////////////////////////////////////////////////////////////////////
SurfaceData getSurfaceData(void)
{
	SurfaceData sd;
    // by material
    // sd.albedo = gl_FrontMaterial.diffuse; 
	// sd.emissive = gl_FrontMaterial.emission;
    // by vertex
	sd.albedo = texture2D(unif_DiffuseMap, var_TexCoord) * gl_Color;
	sd.emissive = vec4(0,0,0,1);
	sd.shininess = unif_Shininess;
	sd.gloss = unif_Gloss;
	sd.normal = normalize(var_Normal);
	
	return sd;
}
