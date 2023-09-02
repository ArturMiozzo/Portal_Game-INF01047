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
uniform int object_id;


uniform vec4 bbox_min;
uniform vec4 bbox_max;

uniform sampler2D TextureFloor;
uniform sampler2D TextureWall;
uniform sampler2D TextureRoof;
uniform sampler2D TexturePortalGun;
uniform sampler2D TexturePortalBlue;
uniform sampler2D TexturePortalOrange;

// Atributos de vértice que serão gerados como saída ("out") pelo Vertex Shader.
// ** Estes serão interpolados pelo rasterizador! ** gerando, assim, valores
// para cada fragmento, os quais serão recebidos como entrada pelo Fragment
// Shader. Veja o arquivo "shader_fragment.glsl".
out vec4 position_world;
out vec4 position_model;
out vec4 normal;
out vec2 texcoords;
out vec3 cor_v;

#define FLOOR 0
#define WALL  1
#define ROOF  2
#define PORTALGUN  3
#define PORTAL1  4
#define PORTAL2  5
#define AIMLEFT  6
#define AIMRIGHT  7

void main()
{
    // A variável gl_Position define a posição final de cada vértice
    // OBRIGATORIAMENTE em "normalized device coordinates" (NDC), onde cada
    // coeficiente estará entre -1 e 1 após divisão por w.
    // Veja {+NDC2+}.
    //
    // O código em "main.cpp" define os vértices dos modelos em coordenadas
    // locais de cada modelo (array model_coefficients). Abaixo, utilizamos
    // operações de modelagem, definição da câmera, e projeção, para computar
    // as coordenadas finais em NDC (variável gl_Position). Após a execução
    // deste Vertex Shader, a placa de vídeo (GPU) fará a divisão por W. Veja
    // slides 41-67 e 69-86 do documento Aula_09_Projecoes.pdf.

    gl_Position = projection * view * model * model_coefficients;

    // Como as variáveis acima  (tipo vec4) são vetores com 4 coeficientes,
    // também é possível acessar e modificar cada coeficiente de maneira
    // independente. Esses são indexados pelos nomes x, y, z, e w (nessa
    // ordem, isto é, 'x' é o primeiro coeficiente, 'y' é o segundo, ...):
    //
    //     gl_Position.x = model_coefficients.x;
    //     gl_Position.y = model_coefficients.y;
    //     gl_Position.z = model_coefficients.z;
    //     gl_Position.w = model_coefficients.w;
    //
    // Agora definimos outros atributos dos vértices que serão interpolados pelo
    // rasterizador para gerar atributos únicos para cada fragmento gerado.

    // Posição do vértice atual no sistema de coordenadas global (World).
    position_world = model * model_coefficients;

    // Posição do vértice atual no sistema de coordenadas local do modelo.
    position_model = model_coefficients;

    // Normal do vértice atual no sistema de coordenadas global (World).
    // Veja slides 123-151 do documento Aula_07_Transformacoes_Geometricas_3D.pdf.
    normal = inverse(transpose(model)) * normal_coefficients;
    normal.w = 0.0;

    // Coordenadas de textura obtidas do arquivo OBJ (se existirem!)
    texcoords = texture_coefficients;

    if(object_id == FLOOR)
    {
        vec4 origin = vec4(0.0, 0.0, 0.0, 1.0);

        vec4 camera_position = inverse(view) * origin;

        vec4 p = position_world;

        vec4 n = normalize(normal);

        vec4 l = normalize(vec4(1.0,1.0,1.0,0.0));

        vec4 v = normalize(camera_position - p);

        vec4 r = -1*l+2*n*(dot(n,l));
        // Coordenadas de textura do plano, obtidas do arquivo OBJ.
        /*float U = texcoords.x*5 - floor(texcoords.x*5);
        float V = texcoords.y*5 - floor(texcoords.y*5);
        // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
        vec3 Kd0 = texture(TextureFloor, vec2(U,V)).rgb;*/
        // Propriedades espectrais do chão
        vec3 Kd = vec3(0.2,0.2,0.2);
        vec3 Ks = vec3(0.3,0.3,0.3);
        vec3 Ka = vec3(0.0,0.0,0.0);
        float q = 20.0;

        // Espectro da fonte de iluminação
        vec3 I = vec3(1.0,1.0,1.0);

        // Espectro da luz ambiente
        vec3 Ia = vec3(0.5,0.5,0.5);

        // Termo difuso utilizando a lei dos cossenos de Lambert
        vec3 lambert_diffuse_term = Kd*I*(max(0,dot(n,l)));

        // Termo ambiente
        vec3 ambient_term = Ka*Ia;

        // Termo especular utilizando o modelo de iluminação de Phong
        vec3 phong_specular_term  = Ks*I*pow(max(0,(dot(r,v))),q);

        // Equação de Iluminação
        //float lambert = max(0,dot(n,l));

        cor_v = (lambert_diffuse_term + ambient_term + phong_specular_term)*10;
    }
    else if(object_id == PORTALGUN) //PORTAL GUN
    {
        // Obtemos a refletância difusa a partir da leitura da imagem TextureImage0
        cor_v = texture(TexturePortalGun, texture_coefficients).rgb;
    }
    else cor_v = vec3(0.0, 0.0, 0.0);
}

