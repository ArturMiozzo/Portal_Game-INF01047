#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <iostream>

// Headers abaixo são específicos de C++
#include <map>
#include <stack>
#include <string>
#include <vector>
#include <limits>
#include <fstream>
#include <sstream>
#include <stdexcept>
#include <algorithm>

// Headers das bibliotecas OpenGL
#include <glad/glad.h>   // Criação de contexto OpenGL 3.3
#include <GLFW/glfw3.h>  // Criação de janelas do sistema operacional

// Headers da biblioteca GLM: criação de matrizes e vetores.
#include <glm/mat4x4.hpp>
#include <glm/vec4.hpp>
#include <glm/gtc/type_ptr.hpp>

// Headers da biblioteca para carregar modelos obj
#include <tiny_obj_loader.h>

#include <stb_image.h>

// Headers locais, definidos na pasta "include/"
#include "utils.h"
#include "matrices.h"


// Define as dimensões do circulo
# define CIRCLE_VERTEX  32 // numero de vertices, contando o centro
# define CIRCLE_RADIUS_IN  0 // raio do circulo interno em NDC
# define CIRCLE_RADIUS_OUT  1 // raio do circulo externo em NDC

# define M_PI           3.14159265358979323846  // pi
# define CIRCLE_SIDES  (CIRCLE_VERTEX+2)

# define PortalAnimationSpeed 15.0

#define FLOOR 0
#define WALL  1
#define ROOF  2
#define PORTALGUN  3
#define PORTAL1  4
#define PORTAL2  5
#define AIMLEFT  6
#define AIMRIGHT  7
#define COMPANION_CUBE 8
#define BUTTON 9

// Estrutura que representa um modelo geométrico carregado a partir de um
// arquivo ".obj". Veja https://en.wikipedia.org/wiki/Wavefront_.obj_file .
struct ObjModel
{
    tinyobj::attrib_t                 attrib;
    std::vector<tinyobj::shape_t>     shapes;
    std::vector<tinyobj::material_t>  materials;

    // Este construtor lê o modelo de um arquivo utilizando a biblioteca tinyobjloader.
    // Veja: https://github.com/syoyo/tinyobjloader
    ObjModel(const char* filename, const char* basepath = NULL, bool triangulate = true)
    {
        printf("Carregando objetos do arquivo \"%s\"...\n", filename);

        // Se basepath == NULL, então setamos basepath como o dirname do
        // filename, para que os arquivos MTL sejam corretamente carregados caso
        // estejam no mesmo diretório dos arquivos OBJ.
        std::string fullpath(filename);
        std::string dirname;
        if (basepath == NULL)
        {
            auto i = fullpath.find_last_of("/");
            if (i != std::string::npos)
            {
                dirname = fullpath.substr(0, i+1);
                basepath = dirname.c_str();
            }
        }

        std::string warn;
        std::string err;
        bool ret = tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, filename, basepath, triangulate);

        if (!err.empty())
            fprintf(stderr, "\n%s\n", err.c_str());

        if (!ret)
            throw std::runtime_error("Erro ao carregar modelo.");

        for (size_t shape = 0; shape < shapes.size(); ++shape)
        {
            if (shapes[shape].name.empty())
            {
                fprintf(stderr,
                        "*********************************************\n"
                        "Erro: Objeto sem nome dentro do arquivo '%s'.\n"
                        "Veja https://www.inf.ufrgs.br/~eslgastal/fcg-faq-etc.html#Modelos-3D-no-formato-OBJ .\n"
                        "*********************************************\n",
                    filename);
                throw std::runtime_error("Objeto sem nome.");
            }
            printf("- Objeto '%s'\n", shapes[shape].name.c_str());
        }

        printf("OK.\n");
    }
};

struct bbox
{
    glm::vec4    bbox_min;
    glm::vec4    bbox_max;
    double       angle;
};

// Declaração de funções utilizadas para pilha de matrizes de modelagem.
void PushMatrix(glm::mat4 M);
void PopMatrix(glm::mat4& M);

// Declaração de várias funções utilizadas em main().  Essas estão definidas
// logo após a definição de main() neste arquivo.
void BuildTrianglesAndAddToVirtualScene(ObjModel*); // Constrói representação de um ObjModel como malha de triângulos para renderização
void ComputeNormals(ObjModel* model); // Computa normais de um ObjModel, caso não existam.
void LoadShadersFromFiles(); // Carrega os shaders de vértice e fragmento, criando um programa de GPU
void LoadGouraudShadersFromFiles();
void LoadTextureImage(const char* filename); // Função que carrega imagens de textura
void DrawVirtualObject(const char* object_name); // Desenha um objeto armazenado em g_VirtualScene
GLuint LoadShader_Vertex(const char* filename);   // Carrega um vertex shader
GLuint LoadShader_Fragment(const char* filename); // Carrega um fragment shader
void LoadShader(const char* filename, GLuint shader_id); // Função utilizada pelas duas acima
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id); // Cria um programa de GPU
void PrintObjModelInfo(ObjModel*); // Função para debugging
void BuildTrianglesAndAddToVirtualScene2(char* name, std::vector<GLuint>* indices, std::vector<float>* model_coefficients, std::vector<float>* normal_coefficients, GLenum rendering_mode);
void BuildAim();
void BuildPortal();
void BuildCube();
bool detectColision(glm::vec4 position, glm::vec4 hitbox_min, glm::vec4 hitbox_max);
int CheckLineBox( glm::vec4 B1, glm::vec4 B2, glm::vec4 L1, glm::vec4 vector_view, glm::vec4 &Hit);
double boxAngle(glm::vec4 B1, glm::vec4 B2);
glm::vec3 bezierCurve(std::vector<glm::vec3> points, float time);
float Bernstein(float k, float n, float t);
// Declaração de funções auxiliares para renderizar texto dentro da janela
// OpenGL. Estas funções estão definidas no arquivo "textrendering.cpp".
void TextRendering_Init();
float TextRendering_LineHeight(GLFWwindow* window);
float TextRendering_CharWidth(GLFWwindow* window);
void TextRendering_PrintString(GLFWwindow* window, const std::string &str, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrix(GLFWwindow* window, glm::mat4 M, float x, float y, float scale = 1.0f);
void TextRendering_PrintVector(GLFWwindow* window, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProduct(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductMoreDigits(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);
void TextRendering_PrintMatrixVectorProductDivW(GLFWwindow* window, glm::mat4 M, glm::vec4 v, float x, float y, float scale = 1.0f);

// Funções abaixo renderizam como texto na janela OpenGL algumas matrizes e
// outras informações do programa. Definidas após main().
void TextRendering_ShowModelViewProjection(GLFWwindow* window, glm::mat4 projection, glm::mat4 view, glm::mat4 model, glm::vec4 p_model);
void TextRendering_ShowEulerAngles(GLFWwindow* window);
void TextRendering_ShowProjection(GLFWwindow* window);
void TextRendering_ShowFramesPerSecond(GLFWwindow* window);

// Funções callback para comunicação com o sistema operacional e interação do
// usuário. Veja mais comentários nas definições das mesmas, abaixo.
void FramebufferSizeCallback(GLFWwindow* window, int width, int height);
void ErrorCallback(int error, const char* description);
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mode);
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods);
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos);
//void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset);



// Definimos uma estrutura que armazenará dados necessários para renderizar
// cada objeto da cena virtual.
struct SceneObject
{
    std::string  name;        // Nome do objeto
    size_t       first_index; // Índice do primeiro vértice dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    size_t       num_indices; // Número de índices do objeto dentro do vetor indices[] definido em BuildTrianglesAndAddToVirtualScene()
    GLenum       rendering_mode; // Modo de rasterização (GL_TRIANGLES, GL_TRIANGLE_STRIP, etc.)
    GLuint       vertex_array_object_id; // ID do VAO onde estão armazenados os atributos do modelo
    glm::vec3    bbox_min; // Axis-Aligned Bounding Box do objeto
    glm::vec3    bbox_max;
};

// Abaixo definimos variáveis globais utilizadas em várias funções do código.

// A cena virtual é uma lista de objetos nomeados, guardados em um dicionário
// (map).  Veja dentro da função BuildTrianglesAndAddToVirtualScene() como que são incluídos
// objetos dentro da variável g_VirtualScene, e veja na função main() como
// estes são acessados.
std::map<std::string, SceneObject> g_VirtualScene;

// Pilha que guardará as matrizes de modelagem.
std::stack<glm::mat4>  g_MatrixStack;

float width = 50.0f;
float height = 5.0f;
float spaceDistance = 10.0f;

bool Portal1Created = false;
bool Portal1OnCube = false;
bbox Portal1Bbox;
bool Portal2Created = false;
bool Portal2OnCube = false;
bbox Portal2Bbox;
double lastPortal1Time = 0;
double lastPortal2Time = 0;

glm::vec3 cubePositionOrigin = glm::vec3(0.0f, height/2, -25.0f);
glm::vec3 cubePosition = cubePositionOrigin;
float cubeWidth = 5.0f;

bool blockMove = false;

// Razão de proporção da janela (largura/altura). Veja função FramebufferSizeCallback().
float g_ScreenRatio = 1.0f;

// Ângulos de Euler que controlam a rotação de um dos cubos da cena virtual
float g_AngleX = 0.0f;
float g_AngleY = 0.0f;
float g_AngleZ = 0.0f;

bool isLookAt = false;

// botoes w a s d
bool b_forward = false;
bool b_right = false;
bool b_back = false;
bool b_left = false;
bool noclip = false;
float speed = 25;
// "g_LeftMouseButtonPressed = true" se o usuário está com o botão esquerdo do mouse
// pressionado no momento atual. Veja função MouseButtonCallback().
bool g_LeftMouseButtonPressed = false;
bool g_RightMouseButtonPressed = false; // Análogo para botão direito do mouse
bool g_MiddleMouseButtonPressed = false; // Análogo para botão do meio do mouse

// Variáveis que definem a câmera em coordenadas esféricas, controladas pelo
// usuário através do mouse (veja função CursorPosCallback()). A posição
// efetiva da câmera é calculada dentro da função main(), dentro do loop de
// renderização.
float g_CameraTheta = 0.0f; // Ângulo no plano ZX em relação ao eixo Z
float g_CameraPhi = 0.0f;   // Ângulo em relação ao eixo Y
float g_CameraDistance = width/2; // Distância da câmera para a origem

// Variáveis que controlam rotação do antebraço
float g_ForearmAngleZ = 0.0f;
float g_ForearmAngleX = 0.0f;

// Variáveis que controlam translação do torso
float g_TorsoPositionX = 0.0f;
float g_TorsoPositionY = 0.0f;

// Variável que controla o tipo de projeção utilizada: perspectiva ou ortográfica.
bool g_UsePerspectiveProjection = true;

// Variável que controla se o texto informativo será mostrado na tela.
bool g_ShowInfoText = true;

// Variáveis que definem um programa de GPU (shaders). Veja função LoadShadersFromFiles().
GLuint g_GpuProgramID = 0;
GLint g_model_uniform;
GLint g_view_uniform;
GLint g_projection_uniform;
GLint g_object_id_uniform;
GLint g_bbox_min_uniform;
GLint g_bbox_max_uniform;
GLint g_light_position_uniform;

// Número de texturas carregadas pela função LoadTextureImage()
GLuint g_NumLoadedTextures = 0;

glm::vec4 camera_position_c;
glm::vec4 last_camera_position_c;


bool isHolding = false;
bool dropped = false;
glm::vec4 box_position;

int main(int argc, char* argv[])
{
    // Inicializamos a biblioteca GLFW, utilizada para criar uma janela do
    // sistema operacional, onde poderemos renderizar com OpenGL.
    int success = glfwInit();
    if (!success)
    {
        fprintf(stderr, "ERROR: glfwInit() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos o callback para impressão de erros da GLFW no terminal
    glfwSetErrorCallback(ErrorCallback);

    // Pedimos para utilizar OpenGL versão 3.3 (ou superior)
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);

    #ifdef __APPLE__
    glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, GL_TRUE);
    #endif

    // Pedimos para utilizar o perfil "core", isto é, utilizaremos somente as
    // funções modernas de OpenGL.
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // Criamos uma janela do sistema operacional, com 800 colunas e 600 linhas
    // de pixels, e com título "INF01047 ...".
    GLFWwindow* window;
    window = glfwCreateWindow(1600, 1200, "Portal", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        fprintf(stderr, "ERROR: glfwCreateWindow() failed.\n");
        std::exit(EXIT_FAILURE);
    }

    // Definimos a função de callback que será chamada sempre que o usuário
    // pressionar alguma tecla do teclado ...
    glfwSetKeyCallback(window, KeyCallback);
    // ... ou clicar os botões do mouse ...
    glfwSetMouseButtonCallback(window, MouseButtonCallback);
    // ... ou movimentar o cursor do mouse em cima da janela ...
    glfwSetCursorPosCallback(window, CursorPosCallback);
    // ... ou rolar a "rodinha" do mouse.
    //glfwSetScrollCallback(window, ScrollCallback);

    // Indicamos que as chamadas OpenGL deverão renderizar nesta janela
    glfwMakeContextCurrent(window);

    // Carregamento de todas funções definidas por OpenGL 3.3, utilizando a
    // biblioteca GLAD.
    gladLoadGLLoader((GLADloadproc) glfwGetProcAddress);

    // Definimos a função de callback que será chamada sempre que a janela for
    // redimensionada, por consequência alterando o tamanho do "framebuffer"
    // (região de memória onde são armazenados os pixels da imagem).
    glfwSetFramebufferSizeCallback(window, FramebufferSizeCallback);
    FramebufferSizeCallback(window, 1600, 1200); // Forçamos a chamada do callback acima, para definir g_ScreenRatio.

    // Imprimimos no terminal informações sobre a GPU do sistema
    const GLubyte *vendor      = glGetString(GL_VENDOR);
    const GLubyte *renderer    = glGetString(GL_RENDERER);
    const GLubyte *glversion   = glGetString(GL_VERSION);
    const GLubyte *glslversion = glGetString(GL_SHADING_LANGUAGE_VERSION);

    printf("GPU: %s, %s, OpenGL %s, GLSL %s\n", vendor, renderer, glversion, glslversion);

    // Carregamos os shaders de vértices e de fragmentos que serão utilizados
    // para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
    //
    LoadShadersFromFiles();
    //LoadGouraudShadersFromFiles();
    // Carregamos duas imagens para serem utilizadas como textura
    LoadTextureImage("../../data/floor.jpg");      // TextureImage0
    LoadTextureImage("../../data/wall.jpg");      // TextureImage1
    LoadTextureImage("../../data/hard_wall.jpg");      // TextureImage2
    LoadTextureImage("../../data/portalgun_col.jpg");      // TextureImage2
    LoadTextureImage("../../data/portal_blue.jpg");      // TextureImage2
    LoadTextureImage("../../data/portal_orange.jpg");      // TextureImage2
    LoadTextureImage("../../data/metal_box.png");
    LoadTextureImage("../../data/Button.bmp");


    // Construímos a representação de objetos geométricos através de malhas de triângulos
    ObjModel floormodel("../../data/floor.obj");
    ComputeNormals(&floormodel);
    BuildTrianglesAndAddToVirtualScene(&floormodel);

    ObjModel wallmodel("../../data/wall.obj");
    ComputeNormals(&wallmodel);
    BuildTrianglesAndAddToVirtualScene(&wallmodel);

    ObjModel roofmodel("../../data/roof.obj");
    ComputeNormals(&roofmodel);
    BuildTrianglesAndAddToVirtualScene(&roofmodel);

    //LoadGouraudShadersFromFiles();
    ObjModel gunmodel("../../data/Portal Gun.obj");
    ComputeNormals(&gunmodel);
    BuildTrianglesAndAddToVirtualScene(&gunmodel);

    ObjModel boxmodel("../../data/Portal_Companion_Cube.obj");
    ComputeNormals(&boxmodel);
    BuildTrianglesAndAddToVirtualScene(&boxmodel);

    ObjModel buttonmodel("../../data/portalbutton.obj");
    ComputeNormals(&buttonmodel);
    BuildTrianglesAndAddToVirtualScene(&buttonmodel);

    BuildAim();
    BuildPortal();
    BuildCube();

    //LoadPhongShadersFromFiles();

    if ( argc > 1 )
    {
        ObjModel model(argv[1]);
        BuildTrianglesAndAddToVirtualScene(&model);
    }

    // Inicializamos o código para renderização de texto.
    TextRendering_Init();

    // Habilitamos o Z-buffer. Veja slides 104-116 do documento Aula_09_Projecoes.pdf.
    glEnable(GL_DEPTH_TEST);

    // Habilitamos o Backface Culling. Veja slides 23-34 do documento Aula_13_Clipping_and_Culling.pdf e slides 112-123 do documento Aula_14_Laboratorio_3_Revisao.pdf.
    glEnable(GL_CULL_FACE);
    glCullFace(GL_BACK);
    glFrontFace(GL_CCW);

    float r = g_CameraDistance;
    camera_position_c  = glm::vec4(0,0,r,1.0f); // Ponto "c", centro da câmera
    // Ficamos em um loop infinito, renderizando, até que o usuário feche a janela

    float t_now;
    float t_prev = glfwGetTime();
    float t_step;

    std::vector<bbox> collisionList;
    std::vector<bbox> portalList;

    bbox wall1;
    wall1.bbox_min = glm::vec4(-width, 0, -width, 0);
    wall1.bbox_max = glm::vec4(width, height, -width, 0);
    wall1.angle = boxAngle(wall1.bbox_min, wall1.bbox_max);

    bbox wall2;
    wall2.bbox_min = glm::vec4(-width, 0, spaceDistance, 0);
    wall2.bbox_max = glm::vec4(-width, height, width, 0);
    wall2.angle = boxAngle(wall2.bbox_min, wall2.bbox_max);

    bbox wall3;
    wall3.bbox_min = glm::vec4(-width, 0, width, 0);
    wall3.bbox_max = glm::vec4(width, height, width, 0);
    wall3.angle = boxAngle(wall3.bbox_min, wall3.bbox_max);

    bbox wall4;
    wall4.bbox_min = glm::vec4(width, 0, spaceDistance, 0);
    wall4.bbox_max = glm::vec4(width, height, width, 0);
    wall4.angle = boxAngle(wall4.bbox_min, wall4.bbox_max);

    bbox wall5;
    wall5.bbox_min = glm::vec4(-width, 0, -width, 0);
    wall5.bbox_max = glm::vec4(-width, height, -spaceDistance, 0);
    wall5.angle = boxAngle(wall5.bbox_min, wall5.bbox_max);

    bbox wall6;
    wall6.bbox_min = glm::vec4(width, 0, -width, 0);
    wall6.bbox_max = glm::vec4(width, height, -spaceDistance, 0);
    wall6.angle = boxAngle(wall6.bbox_min, wall6.bbox_max);

    bbox holeIn;
    holeIn.bbox_min = glm::vec4(-width-1, 0, -spaceDistance, 0);
    holeIn.bbox_max = glm::vec4(width+1, height, -spaceDistance, 0);
    holeIn.angle = boxAngle(holeIn.bbox_min, holeIn.bbox_max);

    bbox holeOut;
    holeOut.bbox_min = glm::vec4(-width-1, 0, spaceDistance, 0);
    holeOut.bbox_max = glm::vec4(width+1, height, spaceDistance, 0);
    holeOut.angle = boxAngle(holeOut.bbox_min, holeOut.bbox_max);

    collisionList.push_back(wall1);
    collisionList.push_back(wall2);
    collisionList.push_back(wall3);
    collisionList.push_back(wall4);
    collisionList.push_back(wall5);
    collisionList.push_back(wall6);
    collisionList.push_back(holeIn);
    collisionList.push_back(holeOut);

    //portalList.push_back(wall1);
    portalList.push_back(wall2);
    portalList.push_back(wall3);
    portalList.push_back(wall4);
    //portalList.push_back(wall5);
    //portalList.push_back(wall6);

    std::vector<glm::vec3> bezierCurvePoints;

    bezierCurvePoints.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
    bezierCurvePoints.push_back(glm::vec3(-0.8f, 0.5f, 0.0f));
    bezierCurvePoints.push_back(glm::vec3(-0.6f, 0.0f, 0.0f));
    bezierCurvePoints.push_back(glm::vec3(-0.4f, 0.5f, 0.0f));
    bezierCurvePoints.push_back(glm::vec3(-0.2f, 0.0f, 0.0f));
    bezierCurvePoints.push_back(glm::vec3(0.0f, 0.5f, 0.0f));

    float t_bezier;
    float t_bezier_last;
    bool isBackwards = false;

    box_position = glm::vec4(+40.0f, -height/2 + 1.25, -30.0f, 1.0f);
    glm::vec4 button_position = glm::vec4(+40.0f, -height/2 + 1, +30.0f, 1.0f);

    /*for(float i=0.0; i<=1.0; i+=0.1)
    {
        PrintVector3(bezierCurve(bezierCurvePoints, i));
    }*/

    glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_HIDDEN); //deixa o cursor invisivel
    while (!glfwWindowShouldClose(window))
    {
        // Aqui executamos as operações de renderização

        // Definimos a cor do "fundo" do framebuffer como branco.  Tal cor é
        // definida como coeficientes RGBA: Red, Green, Blue, Alpha; isto é:
        // Vermelho, Verde, Azul, Alpha (valor de transparência).
        // Conversaremos sobre sistemas de cores nas aulas de Modelos de Iluminação.
        //
        //           R     G     B     A
        glClearColor(1.0f, 1.0f, 1.0f, 1.0f);

        double time = glfwGetTime();

        // "Pintamos" todos os pixels do framebuffer com a cor definida acima,
        // e também resetamos todos os pixels do Z-buffer (depth buffer).
        glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

        // Pedimos para a GPU utilizar o programa de GPU criado acima (contendo
        // os shaders de vértice e fragmentos).
        glUseProgram(g_GpuProgramID);

        // Computamos a posição da câmera utilizando coordenadas esféricas.  As
        // variáveis g_CameraDistance, g_CameraPhi, e g_CameraTheta são
        // controladas pelo mouse do usuário. Veja as funções CursorPosCallback()
        // e ScrollCallback().

        float y = r*sin(g_CameraPhi);
        float z = r*cos(g_CameraPhi)*cos(g_CameraTheta);
        float x = r*cos(g_CameraPhi)*sin(g_CameraTheta);

        //glm::vec4 camera_lookat_l    = glm::vec4(0.0f,0.0f,0.0f,1.0f); // Ponto "l", para onde a câmera (look-at) estará sempre olhando
        //glm::vec4 camera_view_vector = camera_lookat_l - camera_position_c; // Vetor "view", sentido para onde a câmera está virada
        glm::vec4 camera_view_vector = glm::vec4(-x,-y,-z,0.0f);
        glm::vec4 camera_up_vector   = glm::vec4(0.0f,1.0f,0.0f,0.0f); // Vetor "up" fixado para apontar para o "céu" (eito Y global)
        glm::vec4 camera_lookat_l    = glm::vec4(cubePosition.x, cubePosition.y, cubePosition.z, 1.0);

        glm::mat4 view;
        glm::vec4 lastCameraPos;
        t_now = glfwGetTime();
        t_step = t_now - t_prev;

        t_prev = t_now;

        if(isLookAt)
        {
            camera_view_vector = camera_lookat_l - camera_position_c;
            //camera_position_c  = glm::vec4(x,y,z,1.0f);
            view = Matrix_Camera_View_Look_At(camera_position_c, camera_view_vector, camera_up_vector);
        }
        // Computamos a matriz "View" utilizando os parâmetros da câmera para
        // definir o sistema de coordenadas da câmera.  Veja slides 2-14, 184-190 e 236-242 do documento Aula_08_Sistemas_de_Coordenadas.pdf.
        else
        {
            lastCameraPos = glm::vec4(camera_position_c.x,camera_position_c.y,camera_position_c.z,camera_position_c.w);

            view = Matrix_Camera_View(&camera_position_c, camera_view_vector, camera_up_vector, b_forward, b_back, b_right, b_left, speed, noclip, t_step);
            //std::cout << camera_position_c.x << " " << camera_position_c.y << " " << camera_position_c.z << std::endl;

        }
        blockMove = false;

        for (int i=0; i<collisionList.size(); i++)
        {
            bbox wallHitbox;
            wallHitbox.bbox_min = collisionList[i].bbox_min;
            wallHitbox.bbox_max = collisionList[i].bbox_max;

            wallHitbox.bbox_max.x+=1;
            wallHitbox.bbox_min.x-=1;
            wallHitbox.bbox_max.z+=1;
            wallHitbox.bbox_min.z-=1;

            if(detectColision(camera_position_c, wallHitbox.bbox_min, wallHitbox.bbox_max))
            {
                blockMove = true;
            }
        }

        bbox cubeIn;
        cubeIn.bbox_min = glm::vec4(cubePosition.x - cubeWidth/2, cubePosition.y-height/2, cubePosition.z+1, 0);
        cubeIn.bbox_max = glm::vec4(cubePosition.x + cubeWidth/2, cubePosition.y+height/2, cubePosition.z-1, 0);
        cubeIn.angle = 0.0;
        glm::vec4 point;

        if(time - lastPortal1Time > 0.5 && g_LeftMouseButtonPressed)
        {
            if(CheckLineBox(cubeIn.bbox_min, cubeIn.bbox_max,
                            camera_position_c, camera_view_vector, point))
            {
                lastPortal1Time = time;

                float deslX;
                float deslZ;
                float angle = cubeIn.angle;

                if(angle<0.1) angle = 0;

                if(cos(angle)>0.01)
                {
                    deslX = 0;
                    deslZ = 0.01;

                    if(cubeIn.bbox_min.z>0)
                        deslZ = deslZ * -1;
                    else
                        angle = angle + M_PI;
                }
                else
                {
                    deslX = 0.01;
                    deslZ = 0;

                    if(cubeIn.bbox_min.x>0)
                        deslX = deslX * -1;
                    else
                        angle = angle + M_PI;
                }

                Portal1Created = true;
                Portal1OnCube = true;
                Portal1Bbox.bbox_min = glm::vec4(point.x+deslX, height/2, point.z+deslZ, 0.0);
                Portal1Bbox.bbox_max = glm::vec4(point.x-deslX, height/2, point.z-deslZ, 0.0);
                Portal1Bbox.angle = angle;
            }
        }

        if(time - lastPortal2Time > 0.5 && g_RightMouseButtonPressed)
        {

            if(CheckLineBox(cubeIn.bbox_min, cubeIn.bbox_max,
                            camera_position_c, camera_view_vector, point))
            {
                lastPortal2Time = time;

                float deslX;
                float deslZ;
                float angle = cubeIn.angle;

                if(angle<0.1) angle = 0;

                if(cos(angle)>0.01)
                {
                    deslX = 0;
                    deslZ = 0.01;

                    if(cubeIn.bbox_min.z>0)
                        deslZ = deslZ * -1;
                    else
                        angle = angle + M_PI;
                }
                else
                {
                    deslX = 0.01;
                    deslZ = 0;

                    if(cubeIn.bbox_min.x>0)
                        deslX = deslX * -1;
                    else
                        angle = angle + M_PI;
                }

                Portal2Created = true;
                Portal2OnCube = true;
                Portal2Bbox.bbox_min = glm::vec4(point.x+deslX, height/2, point.z+deslZ, 0.0);
                Portal2Bbox.bbox_max = glm::vec4(point.x-deslX, height/2, point.z-deslZ, 0.0);
                Portal2Bbox.angle = angle;
            }
        }

        for (int i=0; i<portalList.size(); i++)
        {
            glm::vec4 point;
            if(time - lastPortal1Time > 0.5 && g_LeftMouseButtonPressed)
            {
                if(CheckLineBox(portalList[i].bbox_min, portalList[i].bbox_max,
                                camera_position_c, camera_view_vector, point))
                {
                    lastPortal1Time = time;

                    float deslX;
                    float deslZ;
                    float angle = portalList[i].angle;

                    printf("angle %f\n", angle);

                    if(angle<0.1) angle = 0;

                    if(cos(angle)>0.01)
                    {
                        deslX = 0;
                        deslZ = 0.01;

                        if(portalList[i].bbox_min.z>0)
                            deslZ = deslZ * -1;
                        else
                            angle = angle + M_PI;
                    }
                    else
                    {
                        deslX = 0.01;
                        deslZ = 0;

                        if(portalList[i].bbox_min.x>0)
                            deslX = deslX * -1;
                        else
                            angle = angle + M_PI;
                    }

                    Portal1Created = true;
                    Portal1OnCube = false;
                    Portal1Bbox.bbox_min = glm::vec4(point.x+deslX, height/2, point.z+deslZ, 0.0);
                    Portal1Bbox.bbox_max = glm::vec4(point.x-deslX, height/2, point.z-deslZ, 0.0);
                    Portal1Bbox.angle = angle;
                }
            }

            if(time - lastPortal2Time > 0.5 && g_RightMouseButtonPressed)
            {

                if(CheckLineBox(portalList[i].bbox_min, portalList[i].bbox_max,
                                camera_position_c, camera_view_vector, point))
                {
                    lastPortal2Time = time;

                    float deslX;
                    float deslZ;
                    float angle = boxAngle(portalList[i].bbox_min, portalList[i].bbox_max);

                    if(angle<0.1) angle = 0;

                    if(cos(angle)>0.01)
                    {
                        deslX = 0;
                        deslZ = 0.01;

                        if(portalList[i].bbox_min.z>0)
                            deslZ = deslZ * -1;
                        else
                            angle = angle + M_PI;
                    }
                    else
                    {
                        deslX = 0.01;
                        deslZ = 0;

                        if(portalList[i].bbox_min.x>0)
                            deslX = deslX * -1;
                        else
                            angle = angle + M_PI;
                    }

                    Portal2Created = true;
                    Portal2OnCube = false;
                    Portal2Bbox.bbox_min = glm::vec4(point.x+deslX, height/2, point.z+deslZ, 0.0);
                    Portal2Bbox.bbox_max = glm::vec4(point.x-deslX, height/2, point.z-deslZ, 0.0);
                    Portal2Bbox.angle = angle;
                }
            }
        }


        // Agora computamos a matriz de Projeção.
        glm::mat4 projection;

        // Note que, no sistema de coordenadas da câmera, os planos near e far
        // estão no sentido negativo! Veja slides 176-204 do documento Aula_09_Projecoes.pdf.
        float nearplane = -0.1f;  // Posição do "near plane"
        float farplane  = -200.0f; // Posição do "far plane"

        if (g_UsePerspectiveProjection)
        {
            // Projeção Perspectiva.
            // Para definição do field of view (FOV), veja slides 205-215 do documento Aula_09_Projecoes.pdf.
            float field_of_view = 3.141592 / 3.0f;
            projection = Matrix_Perspective(field_of_view, g_ScreenRatio, nearplane, farplane);
        }
        else
        {
            // Projeção Ortográfica.
            // Para definição dos valores l, r, b, t ("left", "right", "bottom", "top"),
            // PARA PROJEÇÃO ORTOGRÁFICA veja slides 219-224 do documento Aula_09_Projecoes.pdf.
            // Para simular um "zoom" ortográfico, computamos o valor de "t"
            // utilizando a variável g_CameraDistance.
            float t = 1.5f*g_CameraDistance/2.5f;
            float b = -t;
            float r = t*g_ScreenRatio;
            float l = -r;
            projection = Matrix_Orthographic(l, r, b, t, nearplane, farplane);
        }

        glm::mat4 model = Matrix_Identity(); // Transformação identidade de modelagem

        // Enviamos as matrizes "view" e "projection" para a placa de vídeo
        // (GPU). Veja o arquivo "shader_vertex.glsl", onde estas são
        // efetivamente aplicadas em todos os pontos.
        glUniformMatrix4fv(g_view_uniform       , 1 , GL_FALSE , glm::value_ptr(view));
        glUniformMatrix4fv(g_projection_uniform , 1 , GL_FALSE , glm::value_ptr(projection));
        glUniform4f(g_light_position_uniform, 0.0f, 3.5f, 0.0f, 1.0f);

        glm::mat4 identity = Matrix_Identity();
        glUniformMatrix4fv(g_view_uniform, 1 , GL_FALSE , glm::value_ptr(identity));

        //LoadGouraudShadersFromFiles();
        model = Matrix_Translate(0.2,-0.15,-0.5) * Matrix_Scale(0.2f, 0.2f, 0.2f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, PORTALGUN);
        DrawVirtualObject("PortalGun");
        //LoadPhongShadersFromFiles();
        model = Matrix_Translate(-0.05,0.05,-1) * Matrix_Scale(0.05f, 0.1f, 0.05f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, AIMLEFT);
        DrawVirtualObject("aimLeft");

        model = Matrix_Translate(0.05,-0.05,-1) * Matrix_Scale(0.05f, 0.1f, 0.05f) * Matrix_Rotate(3.141592f, glm::vec4(0.0f,0.0f,1.0f,0.0f));
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, AIMRIGHT);
        DrawVirtualObject("aimRight");

        if(isHolding)
        {
            model = Matrix_Translate(0.0,0.0,-1) * Matrix_Scale(0.7f, 0.7f, 0.7f) * Matrix_Identity();
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, COMPANION_CUBE);
            DrawVirtualObject("pCube2");
            box_position = camera_position_c;
        }
        if(dropped)
        {
            box_position.y = -height/2 +1;
            dropped = false;
            if(isNear(box_position, button_position))
            {
                box_position = button_position;
                box_position.y +=1.25;
                box_position.z +=0.8;
            }
        }


        glUniformMatrix4fv(g_view_uniform, 1 , GL_FALSE , glm::value_ptr(view));

        model = Matrix_Translate(0.0f,-height/2,width/2+spaceDistance/2)* Matrix_Scale(width, height/2, width/2-(spaceDistance/2));// * Matrix_Scale(20.0f, 20.0f, 20.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, FLOOR);
        DrawVirtualObject("the_floor");

        model = Matrix_Translate(0.0f,-height/2,-width/2-spaceDistance/2) * Matrix_Scale(width, height/2, width/2-(spaceDistance/2));// * Matrix_Scale(20.0f, 20.0f, 20.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, FLOOR);
        DrawVirtualObject("the_floor");

        model = Matrix_Translate(0.0f,-5*height/2,0.0f) * Matrix_Scale(width, height/2, spaceDistance);// * Matrix_Scale(20.0f, 20.0f, 20.0f);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, FLOOR);
        DrawVirtualObject("the_floor");

        model = Matrix_Translate(0.0f,height/2,-width) * Matrix_Scale(width, height, 0) * Matrix_Rotate(3.141592f / 2.0f, glm::vec4(1.0f,0.0f,0.0f,0.0f));
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_wall");

        model = Matrix_Translate(0.0f,-3*height/2,-spaceDistance) * Matrix_Scale(width, height, 0) * Matrix_Rotate(3.141592f / 2.0f, glm::vec4(1.0f,0.0f,0.0f,0.0f));
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_wall");

        model = Matrix_Translate(0.0f,-3*height/2,spaceDistance) * Matrix_Scale(width, height, 0) * Matrix_Rotate(3.141592f / 2.0f, glm::vec4(-1.0f,0.0f,0.0f,0.0f));
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_wall");

        model = Matrix_Translate(width,-height/2,0.0f) * Matrix_Scale(0, height*2, width) * Matrix_Rotate(3.141592f / 2.0f, glm::vec4(0.0f,-1.0f,0.0f,0.0f)) * Matrix_Rotate(3.141592f / 2.0f, glm::vec4(1.0f,0.0f,0.0f,0.0f));
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_wall");

        model = Matrix_Translate(-width,-height/2,0.0f) * Matrix_Scale(0, height*2, width) * Matrix_Rotate(3.141592f / 2.0f, glm::vec4(0.0f,1.0f,0.0f,0.0f)) * Matrix_Rotate(3.141592f / 2.0f, glm::vec4(1.0f,0.0f,0.0f,0.0f));
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_wall");

        model = Matrix_Translate(0.0f,height/2,width) * Matrix_Scale(width, height, 0) * Matrix_Rotate(3.141592f / 2.0f, glm::vec4(-1.0f,0.0f,0.0f,0.0f));
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, WALL);
        DrawVirtualObject("the_wall");

        model = Matrix_Translate(0.0f,3*height/2,0.0f) * Matrix_Scale(width, height/2, width) * Matrix_Rotate(3.141592f, glm::vec4(1.0f,0.0f,0.0f,0.0f));
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, ROOF);
        DrawVirtualObject("the_roof");

        model = Matrix_Translate(+40.0f, -height/2 + 0.01, +30.0f)* Matrix_Scale(0.07, 0.07, 0.07) * Matrix_Identity();
        glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, BUTTON);
        DrawVirtualObject("Stm_button01");
        DrawVirtualObject("Stm_button02");

        if(!isHolding)
        {
            model = Matrix_Translate(box_position.x, box_position.y, box_position.z + 3)* Matrix_Identity();
            glUniformMatrix4fv(g_model_uniform, 1, GL_FALSE, glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, COMPANION_CUBE);
            DrawVirtualObject("pCube2");
        }


        //printf("%f\n", ((int)(time*1000)%5000)/5000.0);

        t_bezier_last = t_bezier;
        t_bezier = ((int)(time*1000)%5000)/5000.0;
        glm::vec3 bezier_point;
        if(t_bezier_last > t_bezier)
        {
            isBackwards = !isBackwards;
        }
        if(isBackwards)
        {
            bezier_point = bezierCurve(bezierCurvePoints, 1.0 - t_bezier);
        }
        else bezier_point = bezierCurve(bezierCurvePoints, t_bezier);
        //PrintVector(camera_position_c);
        std::cout << isNear(camera_position_c, box_position) << std::endl;
        cubePosition.x=cubePositionOrigin.x+(bezier_point.x*width);
        cubePosition.y=cubePositionOrigin.y+(bezier_point.y*height);
        cubePosition.z=cubePositionOrigin.z+(bezier_point.z*width);

        if(Portal1OnCube)
        {
            Portal1Bbox.bbox_min.x=cubePosition.x;
            Portal1Bbox.bbox_min.y=cubePosition.y;
            Portal1Bbox.bbox_min.z=cubePosition.z+1.01;
            Portal1Bbox.bbox_max.x=cubePosition.x;
            Portal1Bbox.bbox_max.y=cubePosition.y;
            Portal1Bbox.bbox_max.z=cubePosition.z+1.01;
        }

        if(Portal2OnCube)
        {
            Portal2Bbox.bbox_min.x=cubePosition.x;
            Portal2Bbox.bbox_min.y=cubePosition.y;
            Portal2Bbox.bbox_min.z=cubePosition.z+1.01;
            Portal2Bbox.bbox_max.x=cubePosition.x;
            Portal2Bbox.bbox_max.y=cubePosition.y;
            Portal2Bbox.bbox_max.z=cubePosition.z+1.01;
        }

        model = Matrix_Translate(cubePosition.x,cubePosition.y,cubePosition.z) * Matrix_Scale(cubeWidth, height, 1);
        glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
        glUniform1i(g_object_id_uniform, ROOF);
        DrawVirtualObject("cube");

        if(Portal1Created)
        {
            bbox hitBoxPortal1;
            hitBoxPortal1.bbox_min = Portal1Bbox.bbox_min;
            hitBoxPortal1.bbox_max = Portal1Bbox.bbox_max;

            int deslX;
            int deslZ;

            if(abs(cos(Portal1Bbox.angle))>0.01)
            {
                deslX = 5;
                deslZ = 1;
            }
            else
            {
                deslX = 1;
                deslZ = 5;
            }

            hitBoxPortal1.bbox_min.x -= deslX;
            hitBoxPortal1.bbox_max.x += deslX;
            hitBoxPortal1.bbox_min.z -= deslZ;
            hitBoxPortal1.bbox_max.z += deslZ;
            hitBoxPortal1.bbox_min.y = 0;
            hitBoxPortal1.bbox_max.y = height;

            if(detectColision(camera_position_c, hitBoxPortal1.bbox_min, hitBoxPortal1.bbox_max))
            {
                if(Portal2Created)
                {
                    blockMove = false;

                    float angleCorrection = (Portal1Bbox.angle - Portal2Bbox.angle) + M_PI * cos(Portal1Bbox.angle - Portal2Bbox.angle);

                    if(angleCorrection == -0.0) angleCorrection = 0.0;

                    if(abs(cos(Portal2Bbox.angle))>0.01)
                    {
                        deslX = 0;
                        deslZ = 5;

                        if(Portal2Bbox.bbox_min.z>0)
                            deslZ = deslZ * -1;
                    }
                    else
                    {
                        deslX = 5;
                        deslZ = 0;

                        if(Portal2Bbox.bbox_min.x>0)
                            deslX = deslX * -1;
                    }

                    camera_position_c.x = Portal2Bbox.bbox_min.x + deslX;
                    camera_position_c.z = Portal2Bbox.bbox_min.z + deslZ;

                    g_CameraTheta = g_CameraTheta + angleCorrection;
                }
            }

            model = Matrix_Translate(Portal1Bbox.bbox_min.x,Portal1Bbox.bbox_min.y,Portal1Bbox.bbox_min.z)
             * Matrix_Rotate(Portal1Bbox.angle, glm::vec4(0.0f,1.0f,0.0f,0.0f))
             * Matrix_Scale(std::min((time - lastPortal1Time)*PortalAnimationSpeed, 5.0), std::min((time - lastPortal1Time)*PortalAnimationSpeed, 5.0), 1);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, PORTAL1);
            DrawVirtualObject("Portal1");
        }

        if(Portal2Created)
        {
            bbox hitBoxPortal2;
            hitBoxPortal2.bbox_min = Portal2Bbox.bbox_min;
            hitBoxPortal2.bbox_max = Portal2Bbox.bbox_max;

            int deslX;
            int deslZ;
            if(abs(cos(Portal2Bbox.angle))>0.01)
            {
                deslX = 5;
                deslZ = 1;
            }
            else
            {
                deslX = 1;
                deslZ = 5;
            }

            hitBoxPortal2.bbox_min.x -= deslX;
            hitBoxPortal2.bbox_max.x += deslX;
            hitBoxPortal2.bbox_min.z -= deslZ;
            hitBoxPortal2.bbox_max.z += deslZ;
            hitBoxPortal2.bbox_min.y = 0;
            hitBoxPortal2.bbox_max.y = height;

            if(detectColision(camera_position_c, hitBoxPortal2.bbox_min, hitBoxPortal2.bbox_max))
            {
               if(Portal1Created)
                {
                    blockMove = false;

                    float angleCorrection = (Portal2Bbox.angle - Portal1Bbox.angle) + M_PI * cos(Portal2Bbox.angle - Portal1Bbox.angle);

                    if(angleCorrection == -0.0) angleCorrection = 0.0;

                    if(abs(cos(Portal1Bbox.angle))>0.01)
                    {
                        deslX = 0;
                        deslZ = 5;

                        if(Portal1Bbox.bbox_min.z>0)
                            deslZ = deslZ * -1;
                    }
                    else
                    {
                        deslX = 5;
                        deslZ = 0;

                        if(Portal1Bbox.bbox_min.x>0)
                            deslX = deslX * -1;
                    }

                    camera_position_c.x = Portal1Bbox.bbox_min.x + deslX;
                    camera_position_c.z = Portal1Bbox.bbox_min.z + deslZ;

                    g_CameraTheta = g_CameraTheta + angleCorrection;
                }
            }

            model = Matrix_Translate(Portal2Bbox.bbox_min.x,Portal2Bbox.bbox_min.y,Portal2Bbox.bbox_min.z)
             * Matrix_Rotate(Portal2Bbox.angle, glm::vec4(0.0f,1.0f,0.0f,0.0f))
             * Matrix_Scale(std::min((time - lastPortal2Time)*PortalAnimationSpeed, 5.0), std::min((time - lastPortal2Time)*PortalAnimationSpeed, 5.0), 1);
            glUniformMatrix4fv(g_model_uniform, 1 , GL_FALSE , glm::value_ptr(model));
            glUniform1i(g_object_id_uniform, PORTAL2);
            DrawVirtualObject("Portal2");
        }

        if(blockMove) camera_position_c = lastCameraPos;

        // Imprimimos na tela os ângulos de Euler que controlam a rotação do
        // terceiro cubo.
        TextRendering_ShowEulerAngles(window);

        // Imprimimos na informação sobre a matriz de projeção sendo utilizada.
        TextRendering_ShowProjection(window);

        // Imprimimos na tela informação sobre o número de quadros renderizados
        // por segundo (frames per second).
        TextRendering_ShowFramesPerSecond(window);

        // O framebuffer onde OpenGL executa as operações de renderização não
        // é o mesmo que está sendo mostrado para o usuário, caso contrário
        // seria possível ver artefatos conhecidos como "screen tearing". A
        // chamada abaixo faz a troca dos buffers, mostrando para o usuário
        // tudo que foi renderizado pelas funções acima.
        // Veja o link: https://en.wikipedia.org/w/index.php?title=Multiple_buffering&oldid=793452829#Double_buffering_in_computer_graphics
        glfwSwapBuffers(window);

        // Verificamos com o sistema operacional se houve alguma interação do
        // usuário (teclado, mouse, ...). Caso positivo, as funções de callback
        // definidas anteriormente usando glfwSet*Callback() serão chamadas
        // pela biblioteca GLFW.
        glfwPollEvents();
    }

    // Finalizamos o uso dos recursos do sistema operacional
    glfwTerminate();

    // Fim do programa
    return 0;
}

// Função que carrega uma imagem para ser utilizada como textura
void LoadTextureImage(const char* filename)
{
    printf("Carregando imagem \"%s\"... ", filename);

    // Primeiro fazemos a leitura da imagem do disco
    stbi_set_flip_vertically_on_load(true);
    int width;
    int height;
    int channels;
    unsigned char *data = stbi_load(filename, &width, &height, &channels, 3);

    if ( data == NULL )
    {
        fprintf(stderr, "ERROR: Cannot open image file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }

    printf("OK (%dx%d).\n", width, height);

    // Agora criamos objetos na GPU com OpenGL para armazenar a textura
    GLuint texture_id;
    GLuint sampler_id;
    glGenTextures(1, &texture_id);
    glGenSamplers(1, &sampler_id);

    // Veja slides 95-96 do documento Aula_20_Mapeamento_de_Texturas.pdf
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glSamplerParameteri(sampler_id, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

    // Parâmetros de amostragem da textura.
    glSamplerParameteri(sampler_id, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
    glSamplerParameteri(sampler_id, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

    // Agora enviamos a imagem lida do disco para a GPU
    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
    glPixelStorei(GL_UNPACK_SKIP_PIXELS, 0);
    glPixelStorei(GL_UNPACK_SKIP_ROWS, 0);

    GLuint textureunit = g_NumLoadedTextures;
    glActiveTexture(GL_TEXTURE0 + textureunit);
    glBindTexture(GL_TEXTURE_2D, texture_id);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_SRGB8, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
    glGenerateMipmap(GL_TEXTURE_2D);
    glBindSampler(textureunit, sampler_id);

    stbi_image_free(data);

    g_NumLoadedTextures += 1;
}

int GetIntersection( float fDst1, float fDst2, glm::vec4 P1, glm::vec4 P2, glm::vec4 &Hit) {
if ( (fDst1 * fDst2) >= 0.0f) return 0;
if ( fDst1 == fDst2) return 0;
Hit = P1 + (P2-P1) * ( -fDst1/(fDst2-fDst1) );
return 1;
}

int InBox( glm::vec4 Hit, glm::vec4 B1, glm::vec4 B2, const int Axis) {
if ( Axis==1 && Hit.z > B1.z && Hit.z < B2.z && Hit.y > B1.y && Hit.y < B2.y) return 1;
if ( Axis==2 && Hit.z > B1.z && Hit.z < B2.z && Hit.x > B1.x && Hit.x < B2.x) return 1;
if ( Axis==3 && Hit.x > B1.x && Hit.x < B2.x && Hit.y > B1.y && Hit.y < B2.y) return 1;
return 0;
}

// returns true if line (L1, L2) intersects with the box (B1, B2)
// returns intersection point in Hit
int CheckLineBox( glm::vec4 B1, glm::vec4 B2, glm::vec4 L1, glm::vec4 vector_view, glm::vec4 &Hit)
{
    glm::vec4 L2 = 500.0f * vector_view + L1;

if (L2.x < B1.x && L1.x < B1.x) return false;
if (L2.x > B2.x && L1.x > B2.x) return false;
if (L2.y < B1.y && L1.y < B1.y) return false;
if (L2.y > B2.y && L1.y > B2.y) return false;
if (L2.z < B1.z && L1.z < B1.z) return false;
if (L2.z > B2.z && L1.z > B2.z) return false;
if (L1.x > B1.x && L1.x < B2.x &&
    L1.y > B1.y && L1.y < B2.y &&
    L1.z > B1.z && L1.z < B2.z)
    {Hit = L1;
    return true;}
if ( (GetIntersection( L1.x-B1.x, L2.x-B1.x, L1, L2, Hit) && InBox( Hit, B1, B2, 1 ))
  || (GetIntersection( L1.y-B1.y, L2.y-B1.y, L1, L2, Hit) && InBox( Hit, B1, B2, 2 ))
  || (GetIntersection( L1.z-B1.z, L2.z-B1.z, L1, L2, Hit) && InBox( Hit, B1, B2, 3 ))
  || (GetIntersection( L1.x-B2.x, L2.x-B2.x, L1, L2, Hit) && InBox( Hit, B1, B2, 1 ))
  || (GetIntersection( L1.y-B2.y, L2.y-B2.y, L1, L2, Hit) && InBox( Hit, B1, B2, 2 ))
  || (GetIntersection( L1.z-B2.z, L2.z-B2.z, L1, L2, Hit) && InBox( Hit, B1, B2, 3 )))
	return true;

return false;
}

bool detectColision(glm::vec4 position, glm::vec4 hitbox_min, glm::vec4 hitbox_max)
{
    if (position.x < hitbox_min.x || position.x > hitbox_max.x)
        return false; // No collision along X-axis

    if (position.y < hitbox_min.y || position.y > hitbox_max.y)
        return false; // No collision along Y-axis

    if (position.z < hitbox_min.z || position.z > hitbox_max.z)
        return false; // No collision along Z-axis

    return true;

    /*for (std::map<std::string, SceneObject>::iterator it = g_VirtualScene.begin(); it != g_VirtualScene.end(); it++)
    {
        SceneObject obj = it->second;
        printf("\n%s: ", ((std::string)it->first).c_str());
        printf("bboxmin x:%f y:%f z:%f ", obj.bbox_min.x, obj.bbox_min.y, obj.bbox_min.z);
        printf("bboxmax x:%f y:%f z:%f ", obj.bbox_max.x, obj.bbox_max.y, obj.bbox_max.z);
    }*/
}

double boxAngle(glm::vec4 B1, glm::vec4 B2)
{
    return atan2(B2.y - B1.y, B2.x - B1.x);
}

int factorial(int n)
{
    int f = 1;
    for (int i=1; i<=n; ++i)
        f *= i;
    return f;
}

float Bernstein(float k, float n, float t)
{
    return (factorial(n) / (factorial(k) * factorial(n-k)))*pow(t, k)*pow(1-t, n-k);
}

glm::vec3 bezierCurve(std::vector<glm::vec3> points, float time)
{
    glm::vec3 position = glm::vec3(0.0f, 0.0f, 0.0f);

    for(int i=0; i<points.size(); i++)
    {
        position+=Bernstein(i, points.size(), time) * points.at(i);
    }

    return position;
}

// Função que desenha um objeto armazenado em g_VirtualScene. Veja definição
// dos objetos na função BuildTrianglesAndAddToVirtualScene().
void DrawVirtualObject(const char* object_name)
{
    // "Ligamos" o VAO. Informamos que queremos utilizar os atributos de
    // vértices apontados pelo VAO criado pela função BuildTrianglesAndAddToVirtualScene(). Veja
    // comentários detalhados dentro da definição de BuildTrianglesAndAddToVirtualScene().
    glBindVertexArray(g_VirtualScene[object_name].vertex_array_object_id);

    // Setamos as variáveis "bbox_min" e "bbox_max" do fragment shader
    // com os parâmetros da axis-aligned bounding box (AABB) do modelo.
    glm::vec3 bbox_min = g_VirtualScene[object_name].bbox_min;
    glm::vec3 bbox_max = g_VirtualScene[object_name].bbox_max;
    glUniform4f(g_bbox_min_uniform, bbox_min.x, bbox_min.y, bbox_min.z, 1.0f);
    glUniform4f(g_bbox_max_uniform, bbox_max.x, bbox_max.y, bbox_max.z, 1.0f);
    glUniform4f(g_light_position_uniform, 0.0f, 3.5f, 0.0f, 1.0f);


    // Pedimos para a GPU rasterizar os vértices dos eixos XYZ
    // apontados pelo VAO como linhas. Veja a definição de
    // g_VirtualScene[""] dentro da função BuildTrianglesAndAddToVirtualScene(), e veja
    // a documentação da função glDrawElements() em
    // http://docs.gl/gl3/glDrawElements.
    glDrawElements(
        g_VirtualScene[object_name].rendering_mode,
        g_VirtualScene[object_name].num_indices,
        GL_UNSIGNED_INT,
        (void*)(g_VirtualScene[object_name].first_index * sizeof(GLuint))
    );

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

// Função que carrega os shaders de vértices e de fragmentos que serão
// utilizados para renderização. Veja slides 180-200 do documento Aula_03_Rendering_Pipeline_Grafico.pdf.
//
void LoadShadersFromFiles()
{
    // Note que o caminho para os arquivos "shader_vertex.glsl" e
    // "shader_fragment.glsl" estão fixados, sendo que assumimos a existência
    // da seguinte estrutura no sistema de arquivos:
    //
    //    + FCG_Lab_01/
    //    |
    //    +--+ bin/
    //    |  |
    //    |  +--+ Release/  (ou Debug/ ou Linux/)
    //    |     |
    //    |     o-- main.exe
    //    |
    //    +--+ src/
    //       |
    //       o-- shader_vertex.glsl
    //       |
    //       o-- shader_fragment.glsl
    //
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/shader_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/shader_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");
    g_light_position_uniform = glGetUniformLocation(g_GpuProgramID, "light_position");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureFloor"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureWall"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureRoof"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TexturePortalGun"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TexturePortalBlue"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TexturePortalOrange"), 5);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureCompanionCube"), 6);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureButton"), 7);
    glUseProgram(0);
}
void LoadGouraudShadersFromFiles()
{
    GLuint vertex_shader_id = LoadShader_Vertex("../../src/gouraud_vertex.glsl");
    GLuint fragment_shader_id = LoadShader_Fragment("../../src/gouraud_fragment.glsl");

    // Deletamos o programa de GPU anterior, caso ele exista.
    if ( g_GpuProgramID != 0 )
        glDeleteProgram(g_GpuProgramID);

    // Criamos um programa de GPU utilizando os shaders carregados acima.
    g_GpuProgramID = CreateGpuProgram(vertex_shader_id, fragment_shader_id);

    // Buscamos o endereço das variáveis definidas dentro do Vertex Shader.
    // Utilizaremos estas variáveis para enviar dados para a placa de vídeo
    // (GPU)! Veja arquivo "shader_vertex.glsl" e "shader_fragment.glsl".
    g_model_uniform      = glGetUniformLocation(g_GpuProgramID, "model"); // Variável da matriz "model"
    g_view_uniform       = glGetUniformLocation(g_GpuProgramID, "view"); // Variável da matriz "view" em shader_vertex.glsl
    g_projection_uniform = glGetUniformLocation(g_GpuProgramID, "projection"); // Variável da matriz "projection" em shader_vertex.glsl
    g_object_id_uniform  = glGetUniformLocation(g_GpuProgramID, "object_id"); // Variável "object_id" em shader_fragment.glsl
    g_bbox_min_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_min");
    g_bbox_max_uniform   = glGetUniformLocation(g_GpuProgramID, "bbox_max");

    // Variáveis em "shader_fragment.glsl" para acesso das imagens de textura
    glUseProgram(g_GpuProgramID);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureFloor"), 0);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureWall"), 1);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureRoof"), 2);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TexturePortalGun"), 3);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TexturePortalBlue"), 4);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TexturePortalOrange"), 5);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureCompanionCube"), 6);
    glUniform1i(glGetUniformLocation(g_GpuProgramID, "TextureButton"), 7);
    glUseProgram(0);
}
// Função que pega a matriz M e guarda a mesma no topo da pilha
void PushMatrix(glm::mat4 M)
{
    g_MatrixStack.push(M);
}

// Função que remove a matriz atualmente no topo da pilha e armazena a mesma na variável M
void PopMatrix(glm::mat4& M)
{
    if ( g_MatrixStack.empty() )
    {
        M = Matrix_Identity();
    }
    else
    {
        M = g_MatrixStack.top();
        g_MatrixStack.pop();
    }
}

// Função que computa as normais de um ObjModel, caso elas não tenham sido
// especificadas dentro do arquivo ".obj"
void ComputeNormals(ObjModel* model)
{
    if ( !model->attrib.normals.empty() )
        return;

    // Primeiro computamos as normais para todos os TRIÂNGULOS.
    // Segundo, computamos as normais dos VÉRTICES através do método proposto
    // por Gouraud, onde a normal de cada vértice vai ser a média das normais de
    // todas as faces que compartilham este vértice.

    size_t num_vertices = model->attrib.vertices.size() / 3;

    std::vector<int> num_triangles_per_vertex(num_vertices, 0);
    std::vector<glm::vec4> vertex_normals(num_vertices, glm::vec4(0.0f,0.0f,0.0f,0.0f));

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            glm::vec4  vertices[3];
            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                vertices[vertex] = glm::vec4(vx,vy,vz,1.0);
            }

            const glm::vec4  a = vertices[0];
            const glm::vec4  b = vertices[1];
            const glm::vec4  c = vertices[2];

            const glm::vec4  n = crossproduct(b-a,c-a);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];
                num_triangles_per_vertex[idx.vertex_index] += 1;
                vertex_normals[idx.vertex_index] += n;
                model->shapes[shape].mesh.indices[3*triangle + vertex].normal_index = idx.vertex_index;
            }
        }
    }

    model->attrib.normals.resize( 3*num_vertices );

    for (size_t i = 0; i < vertex_normals.size(); ++i)
    {
        glm::vec4 n = vertex_normals[i] / (float)num_triangles_per_vertex[i];
        n /= norm(n);
        model->attrib.normals[3*i + 0] = n.x;
        model->attrib.normals[3*i + 1] = n.y;
        model->attrib.normals[3*i + 2] = n.z;
    }
}

// Constrói triângulos para futura renderização a partir de um ObjModel.
void BuildTrianglesAndAddToVirtualScene(ObjModel* model)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    std::vector<GLuint> indices;
    std::vector<float>  model_coefficients;
    std::vector<float>  normal_coefficients;
    std::vector<float>  texture_coefficients;

    for (size_t shape = 0; shape < model->shapes.size(); ++shape)
    {
        size_t first_index = indices.size();
        size_t num_triangles = model->shapes[shape].mesh.num_face_vertices.size();

        const float minval = std::numeric_limits<float>::min();
        const float maxval = std::numeric_limits<float>::max();

        glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
        glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

        for (size_t triangle = 0; triangle < num_triangles; ++triangle)
        {
            assert(model->shapes[shape].mesh.num_face_vertices[triangle] == 3);

            for (size_t vertex = 0; vertex < 3; ++vertex)
            {
                tinyobj::index_t idx = model->shapes[shape].mesh.indices[3*triangle + vertex];

                indices.push_back(first_index + 3*triangle + vertex);

                const float vx = model->attrib.vertices[3*idx.vertex_index + 0];
                const float vy = model->attrib.vertices[3*idx.vertex_index + 1];
                const float vz = model->attrib.vertices[3*idx.vertex_index + 2];
                //printf("tri %d vert %d = (%.2f, %.2f, %.2f)\n", (int)triangle, (int)vertex, vx, vy, vz);
                model_coefficients.push_back( vx ); // X
                model_coefficients.push_back( vy ); // Y
                model_coefficients.push_back( vz ); // Z
                model_coefficients.push_back( 1.0f ); // W

                bbox_min.x = std::min(bbox_min.x, vx);
                bbox_min.y = std::min(bbox_min.y, vy);
                bbox_min.z = std::min(bbox_min.z, vz);
                bbox_max.x = std::max(bbox_max.x, vx);
                bbox_max.y = std::max(bbox_max.y, vy);
                bbox_max.z = std::max(bbox_max.z, vz);

                // Inspecionando o código da tinyobjloader, o aluno Bernardo
                // Sulzbach (2017/1) apontou que a maneira correta de testar se
                // existem normais e coordenadas de textura no ObjModel é
                // comparando se o índice retornado é -1. Fazemos isso abaixo.

                if ( idx.normal_index != -1 )
                {
                    const float nx = model->attrib.normals[3*idx.normal_index + 0];
                    const float ny = model->attrib.normals[3*idx.normal_index + 1];
                    const float nz = model->attrib.normals[3*idx.normal_index + 2];
                    normal_coefficients.push_back( nx ); // X
                    normal_coefficients.push_back( ny ); // Y
                    normal_coefficients.push_back( nz ); // Z
                    normal_coefficients.push_back( 0.0f ); // W
                }

                if ( idx.texcoord_index != -1 )
                {
                    const float u = model->attrib.texcoords[2*idx.texcoord_index + 0];
                    const float v = model->attrib.texcoords[2*idx.texcoord_index + 1];
                    texture_coefficients.push_back( u );
                    texture_coefficients.push_back( v );
                }
            }
        }

        size_t last_index = indices.size() - 1;

        SceneObject theobject;
        theobject.name           = model->shapes[shape].name;
        theobject.first_index    = first_index; // Primeiro índice
        theobject.num_indices    = last_index - first_index + 1; // Número de indices
        theobject.rendering_mode = GL_TRIANGLES;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
        theobject.vertex_array_object_id = vertex_array_object_id;

        theobject.bbox_min = bbox_min;
        theobject.bbox_max = bbox_max;

        g_VirtualScene[model->shapes[shape].name] = theobject;
    }

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients.size() * sizeof(float), model_coefficients.data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    if ( !normal_coefficients.empty() )
    {
        GLuint VBO_normal_coefficients_id;
        glGenBuffers(1, &VBO_normal_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, normal_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients.size() * sizeof(float), normal_coefficients.data());
        location = 1; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    if ( !texture_coefficients.empty() )
    {
        GLuint VBO_texture_coefficients_id;
        glGenBuffers(1, &VBO_texture_coefficients_id);
        glBindBuffer(GL_ARRAY_BUFFER, VBO_texture_coefficients_id);
        glBufferData(GL_ARRAY_BUFFER, texture_coefficients.size() * sizeof(float), NULL, GL_STATIC_DRAW);
        glBufferSubData(GL_ARRAY_BUFFER, 0, texture_coefficients.size() * sizeof(float), texture_coefficients.data());
        location = 2; // "(location = 1)" em "shader_vertex.glsl"
        number_of_dimensions = 2; // vec2 em "shader_vertex.glsl"
        glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
        glEnableVertexAttribArray(location);
        glBindBuffer(GL_ARRAY_BUFFER, 0);
    }

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices.size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices.size() * sizeof(GLuint), indices.data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //

    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

void BuildTrianglesAndAddToVirtualScene2(char* name, std::vector<GLuint>* indices, std::vector<float>* model_coefficients, std::vector<float>* normal_coefficients, GLenum rendering_mode)
{
    GLuint vertex_array_object_id;
    glGenVertexArrays(1, &vertex_array_object_id);
    glBindVertexArray(vertex_array_object_id);

    const float minval = std::numeric_limits<float>::min();
    const float maxval = std::numeric_limits<float>::max();

    glm::vec3 bbox_min = glm::vec3(maxval,maxval,maxval);
    glm::vec3 bbox_max = glm::vec3(minval,minval,minval);

    for (size_t vertex = 0; vertex < model_coefficients->size(); vertex+=4)
    {
        float vx = model_coefficients->at(vertex + 0);
        float vy = model_coefficients->at(vertex + 1);
        float vz = model_coefficients->at(vertex + 2);

        bbox_min.x = std::min(bbox_min.x, vx);
        bbox_min.y = std::min(bbox_min.y, vy);
        bbox_min.z = std::min(bbox_min.z, vz);
        bbox_max.x = std::max(bbox_max.x, vx);
        bbox_max.y = std::max(bbox_max.y, vy);
        bbox_max.z = std::max(bbox_max.z, vz);
    }

    SceneObject theobject;
    std::string s(name);
    theobject.name           = s;
    theobject.first_index    = 0; // Primeiro índice
    theobject.num_indices    = indices->size(); // Número de indices
    theobject.rendering_mode = rendering_mode;       // Índices correspondem ao tipo de rasterização GL_TRIANGLES.
    theobject.vertex_array_object_id = vertex_array_object_id;
    theobject.bbox_min = bbox_min;
    theobject.bbox_max = bbox_max;

    g_VirtualScene[name] = theobject;

    GLuint VBO_model_coefficients_id;
    glGenBuffers(1, &VBO_model_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_model_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, model_coefficients->size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, model_coefficients->size() * sizeof(float), model_coefficients->data());
    GLuint location = 0; // "(location = 0)" em "shader_vertex.glsl"
    GLint  number_of_dimensions = 4; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint VBO_normal_coefficients_id;
    glGenBuffers(1, &VBO_normal_coefficients_id);
    glBindBuffer(GL_ARRAY_BUFFER, VBO_normal_coefficients_id);
    glBufferData(GL_ARRAY_BUFFER, normal_coefficients->size() * sizeof(float), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ARRAY_BUFFER, 0, normal_coefficients->size() * sizeof(float), normal_coefficients->data());
    location = 2; // "(location = 1)" em "shader_vertex.glsl"
    number_of_dimensions = 3; // vec4 em "shader_vertex.glsl"
    glVertexAttribPointer(location, number_of_dimensions, GL_FLOAT, GL_FALSE, 0, 0);
    glEnableVertexAttribArray(location);
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    GLuint indices_id;
    glGenBuffers(1, &indices_id);

    // "Ligamos" o buffer. Note que o tipo agora é GL_ELEMENT_ARRAY_BUFFER.
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, indices_id);
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, indices->size() * sizeof(GLuint), NULL, GL_STATIC_DRAW);
    glBufferSubData(GL_ELEMENT_ARRAY_BUFFER, 0, indices->size() * sizeof(GLuint), indices->data());
    // glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0); // XXX Errado!
    //
    // "Desligamos" o VAO, evitando assim que operações posteriores venham a
    // alterar o mesmo. Isso evita bugs.
    glBindVertexArray(0);
}

void BuildAim()
{
    GLfloat model_coefficients_left[] = {
     1.0f, 0.72f, 0.0f, 1.0f,
     0.75f, 0.95f, 0.0f, 1.0f,
     0.56f, 0.82f, 0.0f, 1.0f,
     0.31f, 1.0f, 0.0f, 1.0f,
     0.2f, 0.75f, 0.0f, 1.0f,
     -0.14f, 0.94f, 0.0f, 1.0f,
     -0.16f, 0.53f, 0.0f, 1.0f,
     -0.45f, 0.77f, 0.0f, 1.0f,
     -0.45f, 0.22f, 0.0f, 1.0f,
     -0.72f, 0.51f, 0.0f, 1.0f,
     -0.68f, -0.19f, 0.0f, 1.0f,
     -0.91f, 0.15f, 0.0f, 1.0f,
     -0.97f, -0.11f, 0.0f, 1.0f,
     -0.72f, -0.58f, 0.0f, 1.0f,
     -1.0f, -0.51f, 0.0f, 1.0f,
     -0.89f, -1.0f, 0.0f, 1.0f
    };

    GLfloat model_coefficients_right[] = {
     1.0f, 0.72f, 0.0f, 1.0f,
     0.75f, 0.95f, 0.0f, 1.0f,
     0.56f, 0.82f, 0.0f, 1.0f,
     0.31f, 1.0f, 0.0f, 1.0f,
     0.2f, 0.75f, 0.0f, 1.0f,
     -0.14f, 0.94f, 0.0f, 1.0f,
     -0.16f, 0.53f, 0.0f, 1.0f,
     -0.45f, 0.77f, 0.0f, 1.0f,
     -0.45f, 0.22f, 0.0f, 1.0f,
     -0.72f, 0.51f, 0.0f, 1.0f,
     -0.68f, -0.19f, 0.0f, 1.0f,
     -0.91f, 0.15f, 0.0f, 1.0f,
     -0.97f, -0.11f, 0.0f, 1.0f,
     -0.72f, -0.58f, 0.0f, 1.0f,
     -1.0f, -0.51f, 0.0f, 1.0f,
     -0.89f, -1.0f, 0.0f, 1.0f
    };

    GLfloat color_coefficients[] = {
    // Cores dos vértices do cubo
    //  R     G     B
        1.0f, 0.5f, 0.0f, // cor do vértice 0
        1.0f, 0.5f, 0.0f, // cor do vértice 1
        1.0f, 0.5f, 0.0f, // cor do vértice 0
        1.0f, 0.5f, 0.0f, // cor do vértice 1
        1.0f, 0.5f, 0.0f, // cor do vértice 0
        1.0f, 0.5f, 0.0f, // cor do vértice 1
        1.0f, 0.5f, 0.0f, // cor do vértice 1
        1.0f, 0.5f, 0.0f, // cor do vértice 0
        1.0f, 0.5f, 0.0f, // cor do vértice 1
        1.0f, 0.5f, 0.0f, // cor do vértice 0
        1.0f, 0.5f, 0.0f, // cor do vértice 1
        1.0f, 0.5f, 0.0f, // cor do vértice 1
        1.0f, 0.5f, 0.0f, // cor do vértice 0
        1.0f, 0.5f, 0.0f, // cor do vértice 1
        1.0f, 0.5f, 0.0f, // cor do vértice 1
        1.0f, 0.5f, 0.0f, // cor do vértice 1
    };

    GLuint indices[] = {
        0, 1, 2,
        1, 3, 2,
        2, 3, 4,
        3, 5, 4,
        4, 5, 6,
        5, 7, 6,
        6, 7, 8,
        7, 9, 8,
        8, 9, 10,
        9, 11, 10,
        10, 11, 12,
        11, 13, 12,
        12, 13, 14,
        13, 15, 14,
    };

    int n = sizeof(model_coefficients_left) / sizeof(model_coefficients_left[0]);

	std::vector<GLfloat> modelvecLeft(model_coefficients_left, model_coefficients_left + n);

	n = sizeof(model_coefficients_right) / sizeof(model_coefficients_right[0]);

	std::vector<GLfloat> modelvecRight(model_coefficients_right, model_coefficients_right + n);

    n = sizeof(color_coefficients) / sizeof(color_coefficients[0]);

	std::vector<GLfloat> colorvec(color_coefficients, color_coefficients + n);

    n = sizeof(indices) / sizeof(indices[0]);

	std::vector<GLuint> indicesvec(indices, indices + n);

    BuildTrianglesAndAddToVirtualScene2("aimLeft", &indicesvec, &modelvecLeft, &colorvec, GL_TRIANGLES);

    BuildTrianglesAndAddToVirtualScene2("aimRight", &indicesvec, &modelvecRight, &colorvec, GL_TRIANGLES);
}

void BuildCube()
{
    GLfloat model_coefficients[] = {
    // Vértices de um cubo
    //    X      Y     Z     W
        -1.0f,  1.0f,  1.0f, 1.0f, // posição do vértice 0
        -1.0f, -1.0f,  1.0f, 1.0f, // posição do vértice 1
         1.0f, -1.0f,  1.0f, 1.0f, // posição do vértice 2
         1.0f,  1.0f,  1.0f, 1.0f, // posição do vértice 3
        -1.0f,  1.0f, -1.0f, 1.0f, // posição do vértice 4
        -1.0f, -1.0f, -1.0f, 1.0f, // posição do vértice 5
         1.0f, -1.0f, -1.0f, 1.0f, // posição do vértice 6
         1.0f,  1.0f, -1.0f, 1.0f, // posição do vértice 7
    };

    GLfloat color_coefficients[] = {
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 0
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 1
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 2
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 3
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 4
        1.0f, 0.5f, 0.0f, 1.0f, // cor do vértice 5
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 6
        0.0f, 0.5f, 1.0f, 1.0f, // cor do vértice 7
    };

    GLuint indices[] = {
    // Definimos os índices dos vértices que definem as FACES de um cubo
    // através de 12 triângulos que serão desenhados com o modo de renderização
    // GL_TRIANGLES.
        0, 1, 2, // triângulo 1
        7, 6, 5, // triângulo 2
        3, 2, 6, // triângulo 3
        4, 0, 3, // triângulo 4
        4, 5, 1, // triângulo 5
        1, 5, 6, // triângulo 6
        0, 2, 3, // triângulo 7
        7, 5, 4, // triângulo 8
        3, 6, 7, // triângulo 9
        4, 3, 7, // triângulo 10
        4, 1, 0, // triângulo 11
        1, 6, 2, // triângulo 12
    };

    int n = sizeof(model_coefficients) / sizeof(model_coefficients[0]);

	std::vector<GLfloat> modelvec(model_coefficients, model_coefficients + n);

    n = sizeof(color_coefficients) / sizeof(color_coefficients[0]);

	std::vector<GLfloat> colorvec(color_coefficients, color_coefficients + n);

    n = sizeof(indices) / sizeof(indices[0]);

	std::vector<GLuint> indicesvec(indices, indices + n);

    BuildTrianglesAndAddToVirtualScene2("cube", &indicesvec, &modelvec, &colorvec, GL_TRIANGLES);
}

void BuildPortal()
{

    GLfloat NDC_coefficients[CIRCLE_SIDES*4] = { 0.0f };
    GLfloat color_coefficients[CIRCLE_SIDES*4] = { 0.0f };
    for (int i = 0; i < CIRCLE_SIDES; i+=2)
    {
        // calcula a posição em radianos para desenhar o vertice
        float radians = i/(CIRCLE_VERTEX*1.0) * 2 * M_PI;

        // circulo interno
        // x é o seno da posição
        NDC_coefficients[i*4] = sin(radians) * CIRCLE_RADIUS_IN / 2.5;
        // y é o cosseno
        NDC_coefficients[i*4+1] = cos(radians) * CIRCLE_RADIUS_IN;
        // o w é fixo em 1
        NDC_coefficients[i*4+3] = 1.0f;

        // circulo externo
        // x é o seno da posição
        NDC_coefficients[i*4+4] = sin(radians) * CIRCLE_RADIUS_OUT / 2;
        // y é o cosseno
        NDC_coefficients[i*4+5] = cos(radians) * CIRCLE_RADIUS_OUT;
        // o w é fixo em 1
        NDC_coefficients[i*4+7] = 1.0f;

        // todos os vertices são na cor azul, 0,0,1,1
        color_coefficients[i*4] = 1.0f;
        color_coefficients[i*4+3] = 1.0f;
        color_coefficients[i*4+4] = 1.0f;
        color_coefficients[i*4+7] = 1.0f;
    }

    GLubyte indices[CIRCLE_SIDES]; // GLubyte: valores entre 0 e 255 (8 bits sem sinal).

    // no triangle fan, os indices são uma sequencia igual ao numero de lados
    for(int i=0;i<CIRCLE_SIDES;i++)
    {
        indices[i]=i;
    }

    int n = sizeof(NDC_coefficients) / sizeof(NDC_coefficients[0]);

	std::vector<GLfloat> modelvecportal(NDC_coefficients, NDC_coefficients + n);

    n = sizeof(color_coefficients) / sizeof(color_coefficients[0]);

	std::vector<GLfloat> colorvecportal(color_coefficients, color_coefficients + n);

    n = sizeof(indices) / sizeof(indices[0]);

	std::vector<GLuint> indicesvec(indices, indices + n);

    BuildTrianglesAndAddToVirtualScene2("Portal1", &indicesvec, &modelvecportal, &colorvecportal, GL_TRIANGLE_STRIP);
    BuildTrianglesAndAddToVirtualScene2("Portal2", &indicesvec, &modelvecportal, &colorvecportal, GL_TRIANGLE_STRIP);
}

// Carrega um Vertex Shader de um arquivo GLSL. Veja definição de LoadShader() abaixo.
GLuint LoadShader_Vertex(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos vértices.
    GLuint vertex_shader_id = glCreateShader(GL_VERTEX_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, vertex_shader_id);

    // Retorna o ID gerado acima
    return vertex_shader_id;
}

// Carrega um Fragment Shader de um arquivo GLSL . Veja definição de LoadShader() abaixo.
GLuint LoadShader_Fragment(const char* filename)
{
    // Criamos um identificador (ID) para este shader, informando que o mesmo
    // será aplicado nos fragmentos.
    GLuint fragment_shader_id = glCreateShader(GL_FRAGMENT_SHADER);

    // Carregamos e compilamos o shader
    LoadShader(filename, fragment_shader_id);

    // Retorna o ID gerado acima
    return fragment_shader_id;
}

// Função auxilar, utilizada pelas duas funções acima. Carrega código de GPU de
// um arquivo GLSL e faz sua compilação.
void LoadShader(const char* filename, GLuint shader_id)
{
    // Lemos o arquivo de texto indicado pela variável "filename"
    // e colocamos seu conteúdo em memória, apontado pela variável
    // "shader_string".
    std::ifstream file;
    try {
        file.exceptions(std::ifstream::failbit);
        file.open(filename);
    } catch ( std::exception& e ) {
        fprintf(stderr, "ERROR: Cannot open file \"%s\".\n", filename);
        std::exit(EXIT_FAILURE);
    }
    std::stringstream shader;
    shader << file.rdbuf();
    std::string str = shader.str();
    const GLchar* shader_string = str.c_str();
    const GLint   shader_string_length = static_cast<GLint>( str.length() );

    // Define o código do shader GLSL, contido na string "shader_string"
    glShaderSource(shader_id, 1, &shader_string, &shader_string_length);

    // Compila o código do shader GLSL (em tempo de execução)
    glCompileShader(shader_id);

    // Verificamos se ocorreu algum erro ou "warning" durante a compilação
    GLint compiled_ok;
    glGetShaderiv(shader_id, GL_COMPILE_STATUS, &compiled_ok);

    GLint log_length = 0;
    glGetShaderiv(shader_id, GL_INFO_LOG_LENGTH, &log_length);

    // Alocamos memória para guardar o log de compilação.
    // A chamada "new" em C++ é equivalente ao "malloc()" do C.
    GLchar* log = new GLchar[log_length];
    glGetShaderInfoLog(shader_id, log_length, &log_length, log);

    // Imprime no terminal qualquer erro ou "warning" de compilação
    if ( log_length != 0 )
    {
        std::string  output;

        if ( !compiled_ok )
        {
            output += "ERROR: OpenGL compilation of \"";
            output += filename;
            output += "\" failed.\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }
        else
        {
            output += "WARNING: OpenGL compilation of \"";
            output += filename;
            output += "\".\n";
            output += "== Start of compilation log\n";
            output += log;
            output += "== End of compilation log\n";
        }

        fprintf(stderr, "%s", output.c_str());
    }

    // A chamada "delete" em C++ é equivalente ao "free()" do C
    delete [] log;
}

// Esta função cria um programa de GPU, o qual contém obrigatoriamente um
// Vertex Shader e um Fragment Shader.
GLuint CreateGpuProgram(GLuint vertex_shader_id, GLuint fragment_shader_id)
{
    // Criamos um identificador (ID) para este programa de GPU
    GLuint program_id = glCreateProgram();

    // Definição dos dois shaders GLSL que devem ser executados pelo programa
    glAttachShader(program_id, vertex_shader_id);
    glAttachShader(program_id, fragment_shader_id);

    // Linkagem dos shaders acima ao programa
    glLinkProgram(program_id);

    // Verificamos se ocorreu algum erro durante a linkagem
    GLint linked_ok = GL_FALSE;
    glGetProgramiv(program_id, GL_LINK_STATUS, &linked_ok);

    // Imprime no terminal qualquer erro de linkagem
    if ( linked_ok == GL_FALSE )
    {
        GLint log_length = 0;
        glGetProgramiv(program_id, GL_INFO_LOG_LENGTH, &log_length);

        // Alocamos memória para guardar o log de compilação.
        // A chamada "new" em C++ é equivalente ao "malloc()" do C.
        GLchar* log = new GLchar[log_length];

        glGetProgramInfoLog(program_id, log_length, &log_length, log);

        std::string output;

        output += "ERROR: OpenGL linking of program failed.\n";
        output += "== Start of link log\n";
        output += log;
        output += "\n== End of link log\n";

        // A chamada "delete" em C++ é equivalente ao "free()" do C
        delete [] log;

        fprintf(stderr, "%s", output.c_str());
    }

    // Os "Shader Objects" podem ser marcados para deleção após serem linkados
    glDeleteShader(vertex_shader_id);
    glDeleteShader(fragment_shader_id);

    // Retornamos o ID gerado acima
    return program_id;
}

// Definição da função que será chamada sempre que a janela do sistema
// operacional for redimensionada, por consequência alterando o tamanho do
// "framebuffer" (região de memória onde são armazenados os pixels da imagem).
void FramebufferSizeCallback(GLFWwindow* window, int width, int height)
{
    // Indicamos que queremos renderizar em toda região do framebuffer. A
    // função "glViewport" define o mapeamento das "normalized device
    // coordinates" (NDC) para "pixel coordinates".  Essa é a operação de
    // "Screen Mapping" ou "Viewport Mapping" vista em aula ({+ViewportMapping2+}).
    glViewport(0, 0, width, height);

    // Atualizamos também a razão que define a proporção da janela (largura /
    // altura), a qual será utilizada na definição das matrizes de projeção,
    // tal que não ocorra distorções durante o processo de "Screen Mapping"
    // acima, quando NDC é mapeado para coordenadas de pixels. Veja slides 205-215 do documento Aula_09_Projecoes.pdf.
    //
    // O cast para float é necessário pois números inteiros são arredondados ao
    // serem divididos!
    g_ScreenRatio = (float)width / height;
}

// Variáveis globais que armazenam a última posição do cursor do mouse, para
// que possamos calcular quanto que o mouse se movimentou entre dois instantes
// de tempo. Utilizadas no callback CursorPosCallback() abaixo.
double g_LastCursorPosX, g_LastCursorPosY;

// Função callback chamada sempre que o usuário aperta algum dos botões do mouse
void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_LeftMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_LeftMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_LEFT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_LeftMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_RightMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_RightMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_RIGHT && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_RightMouseButtonPressed = false;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_PRESS)
    {
        // Se o usuário pressionou o botão esquerdo do mouse, guardamos a
        // posição atual do cursor nas variáveis g_LastCursorPosX e
        // g_LastCursorPosY.  Também, setamos a variável
        // g_MiddleMouseButtonPressed como true, para saber que o usuário está
        // com o botão esquerdo pressionado.
        glfwGetCursorPos(window, &g_LastCursorPosX, &g_LastCursorPosY);
        g_MiddleMouseButtonPressed = true;
    }
    if (button == GLFW_MOUSE_BUTTON_MIDDLE && action == GLFW_RELEASE)
    {
        // Quando o usuário soltar o botão esquerdo do mouse, atualizamos a
        // variável abaixo para false.
        g_MiddleMouseButtonPressed = false;
    }
}

// Função callback chamada sempre que o usuário movimentar o cursor do mouse em
// cima da janela OpenGL.
void CursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
    // Abaixo executamos o seguinte: caso o botão esquerdo do mouse esteja
    // pressionado, computamos quanto que o mouse se movimento desde o último
    // instante de tempo, e usamos esta movimentação para atualizar os
    // parâmetros que definem a posição da câmera dentro da cena virtual.
    // Assim, temos que o usuário consegue controlar a câmera.
    float dx, dy;
    if(xpos >= 1099 || xpos <= 400)
    {
        if(ypos >= 899 || ypos <= 300)
        {
            glfwSetCursorPos(window, 800, 600);
            dx = 0;
            dy = 0;
            g_LastCursorPosX = 400;
            g_LastCursorPosY = 300;

        }
        else
        {
            glfwSetCursorPos(window, 800, ypos);
            dx = 0;
            g_LastCursorPosX = 800;
            g_LastCursorPosY = ypos;
        }

    }
    else if(ypos >= 899 || ypos <= 300)
    {

            glfwSetCursorPos(window, xpos, 600);
            dy = 0;
            g_LastCursorPosX = xpos;
            g_LastCursorPosY = 600;

    }
    else
    {
        dx = xpos - g_LastCursorPosX;
        dy = ypos - g_LastCursorPosY;


    // Atualizamos as variáveis globais para armazenar a posição atual do
    // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }


    // Atualizamos parâmetros da câmera com os deslocamentos
    g_CameraTheta -= 0.006f*dx;
    g_CameraPhi   += 0.006f*dy;

    // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
    float phimax = 3.141592f/2;
    float phimin = -phimax;

    if (g_CameraPhi > phimax)
        g_CameraPhi = phimax;

    if (g_CameraPhi < phimin)
        g_CameraPhi = phimin;




    /*double centerX = 800 / 2.0;
    double centerY = 600 / 2.0;
    glfwSetCursorPos(window, centerX, centerY);*/
    /*if (g_LeftMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!

        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da câmera com os deslocamentos
        g_CameraTheta -= 0.01f*dx;
        g_CameraPhi   += 0.01f*dy;

        // Em coordenadas esféricas, o ângulo phi deve ficar entre -pi/2 e +pi/2.
        float phimax = 3.141592f/2;
        float phimin = -phimax;

        if (g_CameraPhi > phimax)
            g_CameraPhi = phimax;

        if (g_CameraPhi < phimin)
            g_CameraPhi = phimin;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }*/

    /*if (g_RightMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_ForearmAngleZ -= 0.01f*dx;
        g_ForearmAngleX += 0.01f*dy;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }

    if (g_MiddleMouseButtonPressed)
    {
        // Deslocamento do cursor do mouse em x e y de coordenadas de tela!
        float dx = xpos - g_LastCursorPosX;
        float dy = ypos - g_LastCursorPosY;

        // Atualizamos parâmetros da antebraço com os deslocamentos
        g_TorsoPositionX += 0.01f*dx;
        g_TorsoPositionY -= 0.01f*dy;

        // Atualizamos as variáveis globais para armazenar a posição atual do
        // cursor como sendo a última posição conhecida do cursor.
        g_LastCursorPosX = xpos;
        g_LastCursorPosY = ypos;
    }*/
}

// Função callback chamada sempre que o usuário movimenta a "rodinha" do mouse.
/*void ScrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
    // Atualizamos a distância da câmera para a origem utilizando a
    // movimentação da "rodinha", simulando um ZOOM.
    g_CameraDistance -= 0.1f*yoffset;

    // Uma câmera look-at nunca pode estar exatamente "em cima" do ponto para
    // onde ela está olhando, pois isto gera problemas de divisão por zero na
    // definição do sistema de coordenadas da câmera. Isto é, a variável abaixo
    // nunca pode ser zero. Versões anteriores deste código possuíam este bug,
    // o qual foi detectado pelo aluno Vinicius Fraga (2017/2).
    const float verysmallnumber = std::numeric_limits<float>::epsilon();
    if (g_CameraDistance < verysmallnumber)
        g_CameraDistance = verysmallnumber;
}*/

// Definição da função que será chamada sempre que o usuário pressionar alguma
// tecla do teclado. Veja http://www.glfw.org/docs/latest/input_guide.html#input_key
void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mod)
{
    // ==================
    // Não modifique este loop! Ele é utilizando para correção automatizada dos
    // laboratórios. Deve ser sempre o primeiro comando desta função KeyCallback().
    for (int i = 0; i < 10; ++i)
        if (key == GLFW_KEY_0 + i && action == GLFW_PRESS && mod == GLFW_MOD_SHIFT)
            std::exit(100 + i);
    // ==================

    // Se o usuário pressionar a tecla ESC, fechamos a janela.
    if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
        glfwSetWindowShouldClose(window, GL_TRUE);

    // O código abaixo implementa a seguinte lógica:
    //   Se apertar tecla X       então g_AngleX += delta;
    //   Se apertar tecla shift+X então g_AngleX -= delta;
    //   Se apertar tecla Y       então g_AngleY += delta;
    //   Se apertar tecla shift+Y então g_AngleY -= delta;
    //   Se apertar tecla Z       então g_AngleZ += delta;
    //   Se apertar tecla shift+Z então g_AngleZ -= delta;

    float delta = 3.141592 / 16; // 22.5 graus, em radianos.

    if (key == GLFW_KEY_X && action == GLFW_PRESS)
    {
        g_AngleX += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    if (key == GLFW_KEY_Y && action == GLFW_PRESS)
    {
        g_AngleY += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }
    if (key == GLFW_KEY_Z && action == GLFW_PRESS)
    {
        g_AngleZ += (mod & GLFW_MOD_SHIFT) ? -delta : delta;
    }

    // Se o usuário apertar a tecla espaço, resetamos os ângulos de Euler para zero.
    if (key == GLFW_KEY_SPACE && action == GLFW_PRESS)
    {
        g_AngleX = 0.0f;
        g_AngleY = 0.0f;
        g_AngleZ = 0.0f;
        g_ForearmAngleX = 0.0f;
        g_ForearmAngleZ = 0.0f;
        g_TorsoPositionX = 0.0f;
        g_TorsoPositionY = 0.0f;
    }

    // Se o usuário apertar a tecla P, utilizamos projeção perspectiva.
    if (key == GLFW_KEY_P && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = true;
    }

    // Se o usuário apertar a tecla O, utilizamos projeção ortográfica.
    if (key == GLFW_KEY_O && action == GLFW_PRESS)
    {
        g_UsePerspectiveProjection = false;
    }

    // Se o usuário apertar a tecla H, fazemos um "toggle" do texto informativo mostrado na tela.
    if (key == GLFW_KEY_H && action == GLFW_PRESS)
    {
        g_ShowInfoText = !g_ShowInfoText;
    }

    // Se o usuário apertar a tecla R, recarregamos os shaders dos arquivos "shader_fragment.glsl" e "shader_vertex.glsl".
    if (key == GLFW_KEY_R && action == GLFW_PRESS)
    {
        LoadShadersFromFiles();
        fprintf(stdout,"Shaders recarregados!\n");
        fflush(stdout);
    }

    if (key == GLFW_KEY_W && action == GLFW_PRESS)
    {
        b_forward = true;
    }
    if (key == GLFW_KEY_W && action == GLFW_RELEASE)
    {
        b_forward = false;
    }

    if (key == GLFW_KEY_S && action == GLFW_PRESS)
    {
        b_back = true;
    }
    if (key == GLFW_KEY_S && action == GLFW_RELEASE)
    {
        b_back = false;
    }

    if (key == GLFW_KEY_D && action == GLFW_PRESS)
    {
        b_right = true;
    }
    if (key == GLFW_KEY_D && action == GLFW_RELEASE)
    {
        b_right = false;
    }

    if (key == GLFW_KEY_A && action == GLFW_PRESS)
    {
        b_left = true;
    }
    if (key == GLFW_KEY_A && action == GLFW_RELEASE)
    {
        b_left = false;
    }
    if (key == GLFW_KEY_V && action == GLFW_PRESS)
    {
        noclip = (noclip == false) ? true : false;
    }
    if (key == GLFW_KEY_L && action == GLFW_PRESS)
    {

        if(isLookAt)
        {
            camera_position_c = last_camera_position_c;
            isLookAt = false;
        }
        else
        {
            last_camera_position_c = camera_position_c;
            isLookAt = true;
        }
    }
    if (key == GLFW_KEY_E && action == GLFW_PRESS)
    {
        if(isNear(camera_position_c, box_position))
        {
            isHolding = (isHolding == false) ? true : false;
            if(!isHolding) dropped = true;
        }

    }

}

// Definimos o callback para impressão de erros da GLFW no terminal
void ErrorCallback(int error, const char* description)
{
    fprintf(stderr, "ERROR: GLFW: %s\n", description);
}

// Esta função recebe um vértice com coordenadas de modelo p_model e passa o
// mesmo por todos os sistemas de coordenadas armazenados nas matrizes model,
// view, e projection; e escreve na tela as matrizes e pontos resultantes
// dessas transformações.
void TextRendering_ShowModelViewProjection(
    GLFWwindow* window,
    glm::mat4 projection,
    glm::mat4 view,
    glm::mat4 model,
    glm::vec4 p_model
)
{
    if ( !g_ShowInfoText )
        return;

    glm::vec4 p_world = model*p_model;
    glm::vec4 p_camera = view*p_world;
    glm::vec4 p_clip = projection*p_camera;
    glm::vec4 p_ndc = p_clip / p_clip.w;

    float pad = TextRendering_LineHeight(window);

    TextRendering_PrintString(window, " Model matrix             Model     In World Coords.", -1.0f, 1.0f-pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, model, p_model, -1.0f, 1.0f-2*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-6*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-7*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-8*pad, 1.0f);

    TextRendering_PrintString(window, " View matrix              World     In Camera Coords.", -1.0f, 1.0f-9*pad, 1.0f);
    TextRendering_PrintMatrixVectorProduct(window, view, p_world, -1.0f, 1.0f-10*pad, 1.0f);

    TextRendering_PrintString(window, "                                        |  ", -1.0f, 1.0f-14*pad, 1.0f);
    TextRendering_PrintString(window, "                            .-----------'  ", -1.0f, 1.0f-15*pad, 1.0f);
    TextRendering_PrintString(window, "                            V              ", -1.0f, 1.0f-16*pad, 1.0f);

    TextRendering_PrintString(window, " Projection matrix        Camera                    In NDC", -1.0f, 1.0f-17*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductDivW(window, projection, p_camera, -1.0f, 1.0f-18*pad, 1.0f);

    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    glm::vec2 a = glm::vec2(-1, -1);
    glm::vec2 b = glm::vec2(+1, +1);
    glm::vec2 p = glm::vec2( 0,  0);
    glm::vec2 q = glm::vec2(width, height);

    glm::mat4 viewport_mapping = Matrix(
        (q.x - p.x)/(b.x-a.x), 0.0f, 0.0f, (b.x*p.x - a.x*q.x)/(b.x-a.x),
        0.0f, (q.y - p.y)/(b.y-a.y), 0.0f, (b.y*p.y - a.y*q.y)/(b.y-a.y),
        0.0f , 0.0f , 1.0f , 0.0f ,
        0.0f , 0.0f , 0.0f , 1.0f
    );

    TextRendering_PrintString(window, "                                                       |  ", -1.0f, 1.0f-22*pad, 1.0f);
    TextRendering_PrintString(window, "                            .--------------------------'  ", -1.0f, 1.0f-23*pad, 1.0f);
    TextRendering_PrintString(window, "                            V                           ", -1.0f, 1.0f-24*pad, 1.0f);

    TextRendering_PrintString(window, " Viewport matrix           NDC      In Pixel Coords.", -1.0f, 1.0f-25*pad, 1.0f);
    TextRendering_PrintMatrixVectorProductMoreDigits(window, viewport_mapping, p_ndc, -1.0f, 1.0f-26*pad, 1.0f);
}

// Escrevemos na tela os ângulos de Euler definidos nas variáveis globais
// g_AngleX, g_AngleY, e g_AngleZ.
void TextRendering_ShowEulerAngles(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float pad = TextRendering_LineHeight(window);

    char buffer[80];
    snprintf(buffer, 80, "Euler Angles rotation matrix = Z(%.2f)*Y(%.2f)*X(%.2f)\n", g_AngleZ, g_AngleY, g_AngleX);

    TextRendering_PrintString(window, buffer, -1.0f+pad/10, -1.0f+2*pad/10, 1.0f);
}

// Escrevemos na tela qual matriz de projeção está sendo utilizada.
void TextRendering_ShowProjection(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    if ( g_UsePerspectiveProjection )
        TextRendering_PrintString(window, "Perspective", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
    else
        TextRendering_PrintString(window, "Orthographic", 1.0f-13*charwidth, -1.0f+2*lineheight/10, 1.0f);
}

// Escrevemos na tela o número de quadros renderizados por segundo (frames per
// second).
void TextRendering_ShowFramesPerSecond(GLFWwindow* window)
{
    if ( !g_ShowInfoText )
        return;

    // Variáveis estáticas (static) mantém seus valores entre chamadas
    // subsequentes da função!
    static float old_seconds = (float)glfwGetTime();
    static int   ellapsed_frames = 0;
    static char  buffer[20] = "?? fps";
    static int   numchars = 7;

    ellapsed_frames += 1;

    // Recuperamos o número de segundos que passou desde a execução do programa
    float seconds = (float)glfwGetTime();

    // Número de segundos desde o último cálculo do fps
    float ellapsed_seconds = seconds - old_seconds;

    if ( ellapsed_seconds > 1.0f )
    {
        numchars = snprintf(buffer, 20, "%.2f fps", ellapsed_frames / ellapsed_seconds);

        old_seconds = seconds;
        ellapsed_frames = 0;
    }

    float lineheight = TextRendering_LineHeight(window);
    float charwidth = TextRendering_CharWidth(window);

    TextRendering_PrintString(window, buffer, 1.0f-(numchars + 1)*charwidth, 1.0f-lineheight, 1.0f);
}

// Função para debugging: imprime no terminal todas informações de um modelo
// geométrico carregado de um arquivo ".obj".
// Veja: https://github.com/syoyo/tinyobjloader/blob/22883def8db9ef1f3ffb9b404318e7dd25fdbb51/loader_example.cc#L98
void PrintObjModelInfo(ObjModel* model)
{
  const tinyobj::attrib_t                & attrib    = model->attrib;
  const std::vector<tinyobj::shape_t>    & shapes    = model->shapes;
  const std::vector<tinyobj::material_t> & materials = model->materials;

  printf("# of vertices  : %d\n", (int)(attrib.vertices.size() / 3));
  printf("# of normals   : %d\n", (int)(attrib.normals.size() / 3));
  printf("# of texcoords : %d\n", (int)(attrib.texcoords.size() / 2));
  printf("# of shapes    : %d\n", (int)shapes.size());
  printf("# of materials : %d\n", (int)materials.size());

  for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
    printf("  v[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.vertices[3 * v + 0]),
           static_cast<const double>(attrib.vertices[3 * v + 1]),
           static_cast<const double>(attrib.vertices[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.normals.size() / 3; v++) {
    printf("  n[%ld] = (%f, %f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.normals[3 * v + 0]),
           static_cast<const double>(attrib.normals[3 * v + 1]),
           static_cast<const double>(attrib.normals[3 * v + 2]));
  }

  for (size_t v = 0; v < attrib.texcoords.size() / 2; v++) {
    printf("  uv[%ld] = (%f, %f)\n", static_cast<long>(v),
           static_cast<const double>(attrib.texcoords[2 * v + 0]),
           static_cast<const double>(attrib.texcoords[2 * v + 1]));
  }

  // For each shape
  for (size_t i = 0; i < shapes.size(); i++) {
    printf("shape[%ld].name = %s\n", static_cast<long>(i),
           shapes[i].name.c_str());
    printf("Size of shape[%ld].indices: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.indices.size()));

    size_t index_offset = 0;

    assert(shapes[i].mesh.num_face_vertices.size() ==
           shapes[i].mesh.material_ids.size());

    printf("shape[%ld].num_faces: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.num_face_vertices.size()));

    // For each face
    for (size_t f = 0; f < shapes[i].mesh.num_face_vertices.size(); f++) {
      size_t fnum = shapes[i].mesh.num_face_vertices[f];

      printf("  face[%ld].fnum = %ld\n", static_cast<long>(f),
             static_cast<unsigned long>(fnum));

      // For each vertex in the face
      for (size_t v = 0; v < fnum; v++) {
        tinyobj::index_t idx = shapes[i].mesh.indices[index_offset + v];
        printf("    face[%ld].v[%ld].idx = %d/%d/%d\n", static_cast<long>(f),
               static_cast<long>(v), idx.vertex_index, idx.normal_index,
               idx.texcoord_index);
      }

      printf("  face[%ld].material_id = %d\n", static_cast<long>(f),
             shapes[i].mesh.material_ids[f]);

      index_offset += fnum;
    }

    printf("shape[%ld].num_tags: %lu\n", static_cast<long>(i),
           static_cast<unsigned long>(shapes[i].mesh.tags.size()));
    for (size_t t = 0; t < shapes[i].mesh.tags.size(); t++) {
      printf("  tag[%ld] = %s ", static_cast<long>(t),
             shapes[i].mesh.tags[t].name.c_str());
      printf(" ints: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].intValues.size(); ++j) {
        printf("%ld", static_cast<long>(shapes[i].mesh.tags[t].intValues[j]));
        if (j < (shapes[i].mesh.tags[t].intValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" floats: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].floatValues.size(); ++j) {
        printf("%f", static_cast<const double>(
                         shapes[i].mesh.tags[t].floatValues[j]));
        if (j < (shapes[i].mesh.tags[t].floatValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");

      printf(" strings: [");
      for (size_t j = 0; j < shapes[i].mesh.tags[t].stringValues.size(); ++j) {
        printf("%s", shapes[i].mesh.tags[t].stringValues[j].c_str());
        if (j < (shapes[i].mesh.tags[t].stringValues.size() - 1)) {
          printf(", ");
        }
      }
      printf("]");
      printf("\n");
    }
  }

  for (size_t i = 0; i < materials.size(); i++) {
    printf("material[%ld].name = %s\n", static_cast<long>(i),
           materials[i].name.c_str());
    printf("  material.Ka = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].ambient[0]),
           static_cast<const double>(materials[i].ambient[1]),
           static_cast<const double>(materials[i].ambient[2]));
    printf("  material.Kd = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].diffuse[0]),
           static_cast<const double>(materials[i].diffuse[1]),
           static_cast<const double>(materials[i].diffuse[2]));
    printf("  material.Ks = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].specular[0]),
           static_cast<const double>(materials[i].specular[1]),
           static_cast<const double>(materials[i].specular[2]));
    printf("  material.Tr = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].transmittance[0]),
           static_cast<const double>(materials[i].transmittance[1]),
           static_cast<const double>(materials[i].transmittance[2]));
    printf("  material.Ke = (%f, %f ,%f)\n",
           static_cast<const double>(materials[i].emission[0]),
           static_cast<const double>(materials[i].emission[1]),
           static_cast<const double>(materials[i].emission[2]));
    printf("  material.Ns = %f\n",
           static_cast<const double>(materials[i].shininess));
    printf("  material.Ni = %f\n", static_cast<const double>(materials[i].ior));
    printf("  material.dissolve = %f\n",
           static_cast<const double>(materials[i].dissolve));
    printf("  material.illum = %d\n", materials[i].illum);
    printf("  material.map_Ka = %s\n", materials[i].ambient_texname.c_str());
    printf("  material.map_Kd = %s\n", materials[i].diffuse_texname.c_str());
    printf("  material.map_Ks = %s\n", materials[i].specular_texname.c_str());
    printf("  material.map_Ns = %s\n",
           materials[i].specular_highlight_texname.c_str());
    printf("  material.map_bump = %s\n", materials[i].bump_texname.c_str());
    printf("  material.map_d = %s\n", materials[i].alpha_texname.c_str());
    printf("  material.disp = %s\n", materials[i].displacement_texname.c_str());
    printf("  <<PBR>>\n");
    printf("  material.Pr     = %f\n", materials[i].roughness);
    printf("  material.Pm     = %f\n", materials[i].metallic);
    printf("  material.Ps     = %f\n", materials[i].sheen);
    printf("  material.Pc     = %f\n", materials[i].clearcoat_thickness);
    printf("  material.Pcr    = %f\n", materials[i].clearcoat_thickness);
    printf("  material.aniso  = %f\n", materials[i].anisotropy);
    printf("  material.anisor = %f\n", materials[i].anisotropy_rotation);
    printf("  material.map_Ke = %s\n", materials[i].emissive_texname.c_str());
    printf("  material.map_Pr = %s\n", materials[i].roughness_texname.c_str());
    printf("  material.map_Pm = %s\n", materials[i].metallic_texname.c_str());
    printf("  material.map_Ps = %s\n", materials[i].sheen_texname.c_str());
    printf("  material.norm   = %s\n", materials[i].normal_texname.c_str());
    std::map<std::string, std::string>::const_iterator it(
        materials[i].unknown_parameter.begin());
    std::map<std::string, std::string>::const_iterator itEnd(
        materials[i].unknown_parameter.end());

    for (; it != itEnd; it++) {
      printf("  material.%s = %s\n", it->first.c_str(), it->second.c_str());
    }
    printf("\n");
  }
}

// set makeprg=cd\ ..\ &&\ make\ run\ >/dev/null
// vim: set spell spelllang=pt_br :

