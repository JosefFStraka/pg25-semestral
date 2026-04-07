#version 460 core
layout(location = 0) in vec3 aPosition;
layout(location = 1) in vec3 aNormal;
layout(location = 2) in vec2 aTextureCoords;

uniform mat4 uP_m = mat4(1.0f);
uniform mat4 uM_m = mat4(1.0f);
uniform mat4 uV_m = mat4(1.0f);

out VS_OUT {
    vec3 N;
    vec2 T;
    vec3 V;
} vs_out;

void main()
{
    // Create Model-View matrix
    mat4 MV_m = uV_m * uM_m;

    // Calculate view-space coordinate - in P point 
    // we are computing the color    
    vec4 P = MV_m * vec4(aPosition, 1.0);
    
    // Calculate normal in view space (not accounting for nonh scaling)
    //vs_out.N = transpose(inverse(mat3(MV_m))) * aNormal;
    vs_out.N = mat3(MV_m) * aNormal;
    
    //Pass on texture coords
    vs_out.T = aTextureCoords;
    
    // Calculate view vector (negative of the view-space position)    
    vs_out.V = P.xyz;

    // Outputs the positions/coordinates of all vertices
    gl_Position = uP_m * P;
}
