#version 330 core
// Atributos de vértice recebidos como entrada ("in") pelo Vertex Shader.
// Veja a função BuildTrianglesAndAddToVirtualScene() em "main.cpp".
layout (location = 0) in vec4 model_coefficients;
layout (location = 1) in vec4 normal_coefficients;
layout (location = 2) in vec2 texture_coefficients;

// Matrizes computadas no código C++ e enviadas para a GPU
uniform mat4 model;
uniform mat4 view;
uniform mat4 projection;

//uniform vec4 light_position;
out vec4 position_model;
out vec3 cor_v;
void main()
{
    vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);
    vec4 camera_position = inverse(view) * origin;

    gl_Position = projection * view * model * model_coefficients;
    // Posição do vértice atual no sistema de coordenadas global (World).
    vec4 position_world = model * model_coefficients;
    vec4 normal = inverse(transpose(model)) * normal_coefficients;

    vec4 v = normalize(camera_position - position_world);
    vec4 l = normalize(vec4(1.0,1.0,1.0,0.0));
    vec4 n = normalize(normal);

    vec4 r = -1*l+2*n*(dot(n,l));

    // Espectro da fonte de iluminação
    vec3 I = vec3(1.0,1.0,1.0);

    // Espectro da luz ambiente
    vec3 Ia = vec3(0.5,0.5,0.5);

    vec3 Kd = vec3(1.0,1.0,1.0);
    vec3 Ks = vec3(1.0,1.0,1.0);
    vec3 Ka = vec3(0.1,0.1,0.1);

    // Termo difuso utilizando a lei dos cossenos de Lambert
    vec3 lambert_diffuse_term = Kd*I*(max(0,dot(n,l)));

    // Termo ambiente
    vec3 ambient_term = Ka*Ia;

    int q = 32;

    // Termo especular utilizando o modelo de iluminação de Phong
    vec3 phong_specular_term  = Ks*I*pow(max(0,(dot(r,v))),q);

    cor_v.rgb = (lambert_diffuse_term + ambient_term + phong_specular_term)*10;
    position_model = model_coefficients;
}
