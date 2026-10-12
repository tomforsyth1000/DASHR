
// This is a simple startup Windows & DirectX11 app that you can use as a foundation for your demos or projects.
// Technically this was written by Tom Forsyth (http://eelpi.gotdns.org/ also on BlueSky and Mastodon)
// but I smashed together Omar Cornut's Dear ImGui DX11 sample and Gargaj's 400-line demo together,
// so they get a bunch of original credit. Sorry about the clash of coding styles though!
// 
// Note - it should be very simple to switch to the Dear ImGui SDL3+DirectX version if you wanted SDL3 support as well,
// which you probably do for anything significant.
//
// https://github.com/ocornut/imgui/wiki/Getting-Started
//
// https://gargaj.github.io/demos-for-dummies (note he uses the opposite matrix organisations that I do)


// This removes the #define min/max. Thanks so much for polluting the namespace!
#define NOMINMAX

#include <d3d11.h>
#include <d3dcompiler.h>
#include <tchar.h>
#include <math.h>
#include <string>

// Partly derived from Dear ImGui: standalone example application for Windows API + DirectX 11
//
// Learn about Dear ImGui:
// - FAQ                  https://dearimgui.com/faq
// - Getting Started      https://dearimgui.com/getting-started
// - Documentation        https://dearimgui.com/docs (same as your local docs/ folder).
// - Introduction, links and more at the top of imgui.cpp

#include "imgui.h"
#include "imgui_impl_win32.h"
#include "imgui_impl_dx11.h"

#define YAK_SHIV_MISC_IMPLEMENTATION
#include "yak_shiv_misc.h"
#define YAK_SHIV_MATRIX_IMPLEMENTATION
#include "yak_shiv_matrix.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

// Dear ImGui data
static ID3D11Device*            g_pd3dDevice = nullptr;
static ID3D11DeviceContext*     g_pd3dDeviceContext = nullptr;
static IDXGISwapChain*          g_pSwapChain = nullptr;
static bool                     g_SwapChainOccluded = false;
static UINT                     g_ResizeWidth = 0, g_ResizeHeight = 0;
static ID3D11RenderTargetView*  g_mainRenderTargetView = nullptr;
static bool                     g_showDearImgui = true;
static bool                     g_showIntroWindow = true;

// Forward declarations of helper functions
void RenderDistortionPass();
bool CreateDeviceD3D(HWND hWnd);
void CleanupDeviceD3D();
void CreateRenderTarget();
void CleanupRenderTarget();
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
void CreateModel();
void CreateTextures();
void CreateTeleportEdgefill();
void CreateShaders();
void GenerateTangentSpace();
void CatchUpSimulationTime ( NtpTimeStamp renderTime );

// Gargaj state.
UINT g_CurrentWidth = 0;
UINT g_CurrentHeight = 0;
ID3D11Texture2D* g_DepthStencil = nullptr;
ID3D11DepthStencilView* g_DepthStencilView = nullptr;
ID3D11RasterizerState* g_RasterizerState = nullptr;
ID3D11DepthStencilState* g_DepthStencilState = nullptr;
ID3D11DepthStencilState* g_DepthStencilStateOff = nullptr;
ID3D11SamplerState *g_SamplerLinear = nullptr;
ID3D11SamplerState *g_SamplerPoint = nullptr;
ID3D11BlendState* g_BlendStateAlpha = nullptr;
ID3D11BlendState* g_BlendStateOff = nullptr;
ID3D11Buffer* g_VertexBuffer = nullptr;
ID3D11Buffer* g_IndexBuffer = nullptr;
ID3D11Buffer* g_VertexBufferEdgefill = nullptr;
ID3D11Buffer* g_IndexBufferEdgefill = nullptr;
ID3D11Buffer* g_VertexBufferWireframe = nullptr;
int g_VertexBufferWireframeSize = 0;
ID3D11Texture2D* g_TextureAlbedo = nullptr;
ID3D11Texture2D* g_TextureHeight = nullptr;
ID3D11Texture2D* g_TextureNormal = nullptr;
ID3D11ShaderResourceView* g_TextureAlbedoSRV = nullptr;
ID3D11ShaderResourceView* g_TextureHeightSRV = nullptr;
ID3D11ShaderResourceView* g_TextureNormalSRV = nullptr;
ID3D11Buffer* g_ConstantBuffer = nullptr;

ID3D11Texture2D*          g_TextureSurfaceFromObjectTemp0 = nullptr;
ID3D11Texture2D*          g_TextureSurfaceFromObjectTemp1 = nullptr;
ID3D11Texture2D*          g_TextureSurfaceFromObjectTemp2 = nullptr;
ID3D11Texture2D*          g_TextureSurfaceFromObjectTemp3 = nullptr;
ID3D11Texture2D*          g_TextureSurfaceFromObject0 = nullptr;
ID3D11Texture2D*          g_TextureSurfaceFromObject1 = nullptr;
ID3D11Texture2D*          g_TextureSurfaceFromObject2 = nullptr;
ID3D11Texture2D*          g_TextureSurfaceFromObject3 = nullptr;
ID3D11Texture2D*          g_TextureTeleportMap = nullptr;
ID3D11Texture2D*          g_TextureEdgefillMap = nullptr;
ID3D11ShaderResourceView* g_TextureSurfaceFromObjectTemp0SRV = nullptr;
ID3D11ShaderResourceView* g_TextureSurfaceFromObjectTemp1SRV = nullptr;
ID3D11ShaderResourceView* g_TextureSurfaceFromObjectTemp2SRV = nullptr;
ID3D11ShaderResourceView* g_TextureSurfaceFromObjectTemp3SRV = nullptr;
ID3D11ShaderResourceView* g_TextureSurfaceFromObject0SRV = nullptr;
ID3D11ShaderResourceView* g_TextureSurfaceFromObject1SRV = nullptr;
ID3D11ShaderResourceView* g_TextureSurfaceFromObject2SRV = nullptr;
ID3D11ShaderResourceView* g_TextureSurfaceFromObject3SRV = nullptr;
ID3D11ShaderResourceView* g_TextureTeleportMapSRV = nullptr;
ID3D11ShaderResourceView* g_TextureEdgefillMapSRV = nullptr;
ID3D11RenderTargetView*   g_TextureSurfaceFromObjectTemp0RTV = nullptr;
ID3D11RenderTargetView*   g_TextureSurfaceFromObjectTemp1RTV = nullptr;
ID3D11RenderTargetView*   g_TextureSurfaceFromObjectTemp2RTV = nullptr;
ID3D11RenderTargetView*   g_TextureSurfaceFromObjectTemp3RTV = nullptr;
ID3D11RenderTargetView*   g_TextureSurfaceFromObject0RTV = nullptr;
ID3D11RenderTargetView*   g_TextureSurfaceFromObject1RTV = nullptr;
ID3D11RenderTargetView*   g_TextureSurfaceFromObject2RTV = nullptr;
ID3D11RenderTargetView*   g_TextureSurfaceFromObject3RTV = nullptr;

struct PipelineState
{
    enum
    {
        Pipeline_Main,
        Pipeline_Deform,
        Pipeline_Edgefill,
        Pipeline_Wireframe,

        Pipeline_COUNT
    };

    LPCWSTR filename = nullptr;

    ID3D11VertexShader* vertexShader = nullptr;
    ID3D11PixelShader* pixelShader = nullptr;
    ID3D11InputLayout* inputLayout = nullptr;
};

// These three things need to match - g_VertexInputDesc[], VertexBufferStruct, and VS_INPUT in VertexInput.hlsl
struct VertexBufferStruct
{
    Vec3 Pos;
    Vec3 TexCoord;
    Vec4 BoneWeights; // rather wasteful - could easily be just bytes.
    Vec3 Normal;
    Vec3 Tangent;
    Vec3 Bitangent;
};

static D3D11_INPUT_ELEMENT_DESC g_VertexInputDesc[] =
{
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"TEXCOORD", 1, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 24, D3D11_INPUT_PER_VERTEX_DATA, 0}, // bone weights
    {"TEXCOORD", 2, DXGI_FORMAT_R32G32B32_FLOAT,    0, 40, D3D11_INPUT_PER_VERTEX_DATA, 0}, // normal
    {"TEXCOORD", 3, DXGI_FORMAT_R32G32B32_FLOAT,    0, 52, D3D11_INPUT_PER_VERTEX_DATA, 0}, // tangent
    {"TEXCOORD", 4, DXGI_FORMAT_R32G32B32_FLOAT,    0, 64, D3D11_INPUT_PER_VERTEX_DATA, 0}, // bitangent
};


// These three things need to match - g_VertexInputWireframeDesc[], VertexBufferWireframeStruct, and VS_INPUT_Wireframe in VertexInput.hlsl
struct VertexBufferWireframeStruct
{
    Vec3 Pos;
    Vec4 Colour;
};

static D3D11_INPUT_ELEMENT_DESC g_VertexInputWireframeDesc[] =
{
    {"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT,    0,  0, D3D11_INPUT_PER_VERTEX_DATA, 0},
    {"COLOR",    0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D11_INPUT_PER_VERTEX_DATA, 0},
};

struct EdgeStruct
{
    int v1;
    int v2;
    int v1Prox;
    int v2Prox;
};

// For the wireframe debug view.
EdgeStruct *g_EdgeList;
int g_EdgeListCount = 0;

// All pipelines share the same constant buffer, for simplicity.
char const* g_ConstantBufferSource;
char const* g_VertexInputSource;
char const* g_SharedCode;

PipelineState g_Pipeline[PipelineState::Pipeline_COUNT];

int g_NumVerts = 3*3*2;
int g_NumTris = 2*((2*2)*2 + (2)*4);
VertexBufferStruct* g_VertexData = nullptr;
VertexBufferStruct* g_VertexDataTemp = nullptr;
UINT* g_IndexData = nullptr;
bool g_FreeArrays = false;

int g_NumSegmentsAround = 8;
int g_NumSegmentsLong = 8;
float g_MiddleTubeLength = 4.0f;
float g_MeshRadius = 1.0f;
float g_SurfaceThickness = 1.0f;
int g_TextureSet = 0;
int g_MeshNumber = 0;
bool g_FloodFillTeleportEdgefill = true;

int g_NumSegmentsAround_Current = 0;
int g_NumSegmentsLong_Current = 0;
float g_MiddleTubeLength_Current = 0.0f;
float g_MeshRadius_Current = 0.0f;
float g_SurfaceThickness_Current = 0.0f;
int g_SurfaceFromObjectTextureSize_Current = 0;
int g_TextureSet_Current = 0;
int g_MeshNumber_Current = 0;
bool g_FloodFillTeleportEdgefill_Current = false;
int g_WireframeMode = 0;
float g_WireframeNormalScale = 0.15f;
float g_WireframeTangentScale = 0.02f;
int g_WireframeDebugTriangle[2] = {-1, -1};
bool g_WireframeShowUniqueMapping = false;
float g_WireframeScale = 0.5f;
float g_WireframeXOffset = 0.75f;
float g_WireframeYOffset = 0.5f;

int g_SurfaceFromObjectTextureSize = 256;
int g_SurfaceFromObjectTextureSizePow2 = 8;

VertexBufferWireframeStruct g_WireframeVerts[65536];

// ConstantBufferStruct needs to match between the C version and the shader definition in ConstantBuffer.hlsl
// Must also be a multiple of 16 bytes, and vec2/3/4 must also be aligned.
struct ConstantBufferStruct
{
    // Keep projection and object matrices separate - see note below.
    Mat44 projectionFromCameraMatrix;
    Mat44 cameraFromObjectMatrix;
    Mat44 objectFromCameraMatrix;

    float HeightScale;
    float HeightOffset;
    float StepSize;
    float StepScale;

    Mat44 BoneFromObject[4];

    Dir SunDirInObject;
    float SurfaceFromObjectTextureSize;

    int DebugMode;
    int LightingMode;
    int DistortionMode;
    int MaxSteps;

	float DebugDampingFactor1;
	float DebugDampingFactor2;
	float DebugDampingFactor3;
    float HeightExtraMeshExtrude;

    float DeltaUVStep;
    float ShadowAcneScaler;
    float IndirectLighting;
    float HeightNormalsScale;

    int DebugIterationsAfterTeleport;
	int Padding1;
	int Padding2;
	int Padding3;

} g_ConstantBufferData = {};


enum MeshTypes
{
    MeshType_Tube,
    MeshType_Cube,
    MeshType_TubePinched,
    MeshType_CubePinched,
};

const char* MeshNames[] = {
    "Tube",
    "Inflated cube",
    "Tube with pinched ends",
    "Inflated cube with pinched corners",
};

const char* DebugModeNames[] = {
    "Off",
    "Show heightfield with vertex UVs", // using vertex UVs directly.
    "Show albedo with vertex UVs",      // using vertex UVs directly.
    "Step counts",                      // green = primary step count. Red = shadow step count. Blue = number of teleports.
    "UV grid",                          // UV values of primary hit.
    "Anim Distortion",                  // a measure of the animation distortion.
};

const char* LightingModeNames[] = {
    "Unlit heightfield",
    "SurfaceFromObject normal lit",
    "Heightfield delta lit",
    "Normalmap lit",
    "Normalmap + shadow raytrace"
};

const char* DistortionModeNames[] = {
    "Vanilla mat4x3",
    "ObjectPos + mat3x3"
};

const char* WireframeModeNames[] = {
    "Off",
    "Rendered mesh",
    "Rendered mesh and tangents",
    "Base mesh"
};



// Dear ImGui does have its own internal sense of time, but it's good to
// do this explicitly and have both wall-clock and game time.
NtpTimeStampGetPerformanceCounter g_TimeStampWallClock;
// These are the two in-game clocks - one is the last simulated time, the other is the currently-rendered time.
NtpTimeStamp g_TimeStampGameClockSim;
NtpTimeStamp g_TimeStampGameClockRender;

// It is a very good idea to have a fixed simulation rate.
// Variable sim rates have all sorts of exciting problems.
// Here, just to illustrate things, we allow the simulation steps time to be controlled by an ImGui slider,
// but in practice it would be a compile-time constant.
float g_SimulationTimeStepSeconds = 0.1f;

// Very simple "game" state.
struct GameObject
{
    WorldOrientationAsync worldFromObject;
    Vec3 spin;
    float speed;
};

int const g_NumObjects = 16;
GameObject g_GameObjects[g_NumObjects];

WorldOrientation g_worldFromShape;

// The animation bones.
float g_boneAnimSeconds = 10.0f;
float g_boneAnimSeconds2 = 10.0f;
float g_boneAnimPeriod = 12.0f;
float g_boneAnimAmount = 1.0f;
bool g_boneAnimPaused = false;
Orn g_boneFromObject[4];

float g_sunAnimSeconds = 8.0f;
float g_sunAnimPeriod = 19.0f;
float g_sunAnimHeight = 0.5f;
bool g_sunAnimPaused = true;

// Misc state.
bool g_VsyncEnabled = true;
bool g_GamePaused = false;
// The terminology here is "this matrix transforms points TO world space FROM object space"
// See https://tomforsyth1000.github.io/blog.wiki.html#%5B%5BMatrix%20maths%20and%20names%5D%5D for more details on why this is the Right Scheme.
WorldOrientation g_WorldFromCamera;
WorldPosition g_CameraMayaFocusPos;
float g_MayaFocusDistance = 2.0f;
float g_nearClipPlane = 0.1f;

const char* AlphaModeNames[] = {
    "Alpha test",
    "Alpha test + shader depth (TODO)",
    "Alpha blend + test", // Currently nothing takes advantage of this!
    "Alpha-to-coverage (TODO)",
    "Alpha-to-coverage + shader depth (TODO)",
};

int g_AlphaMode = 0;


// Handy
#define SAFE_RELEASE(thing) if (thing) { thing->Release(); thing = nullptr; }

// Main code
int main(int, char**)
{
    // Make process DPI aware and obtain main monitor scale
    ImGui_ImplWin32_EnableDpiAwareness();
    float main_scale = ImGui_ImplWin32_GetDpiScaleForMonitor(::MonitorFromPoint(POINT{ 0, 0 }, MONITOR_DEFAULTTOPRIMARY));

    // Create application window
    WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, WndProc, 0L, 0L, GetModuleHandle(nullptr), nullptr, nullptr, nullptr, nullptr, L"Skinned Heightfield", nullptr };
    ::RegisterClassExW(&wc);
    HWND hwnd = ::CreateWindowW(wc.lpszClassName, L"Skinned Heightfield", WS_OVERLAPPEDWINDOW, 100, 100, (int)(1280 * main_scale), (int)(720 * main_scale), nullptr, nullptr, wc.hInstance, nullptr);

    // Initialize Direct3D
    if (!CreateDeviceD3D(hwnd))
    {
        CleanupDeviceD3D();
        ::UnregisterClassW(wc.lpszClassName, wc.hInstance);
        return 1;
    }

    // Show the window
    ::ShowWindow(hwnd, SW_SHOWDEFAULT);
    ::UpdateWindow(hwnd);

    // Reset time.
    g_TimeStampWallClock.Init();
    g_TimeStampGameClockSim.Init();
    g_TimeStampGameClockRender.Init();

    // Setup Dear ImGui context
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGuiIO& io = ImGui::GetIO(); (void)io;
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
    io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls

    // Setup Dear ImGui style
    ImGui::StyleColorsDark();
    //ImGui::StyleColorsLight();

    // Setup scaling
    ImGuiStyle& style = ImGui::GetStyle();
    style.ScaleAllSizes(main_scale);        // Bake a fixed style scale. (until we have a solution for dynamic style scaling, changing this requires resetting Style + calling this again)
    style.FontScaleDpi = main_scale;        // Set initial font scale. (in docking branch: using io.ConfigDpiScaleFonts=true automatically overrides this for every window depending on the current monitor)

    // Setup Platform/Renderer backends
    ImGui_ImplWin32_Init(hwnd);
    ImGui_ImplDX11_Init(g_pd3dDevice, g_pd3dDeviceContext);

    // Load Fonts
    // - If fonts are not explicitly loaded, Dear ImGui will select an embedded font: either AddFontDefaultVector() or AddFontDefaultBitmap().
    //   This selection is based on (style.FontSizeBase * style.FontScaleMain * style.FontScaleDpi) reaching a small threshold.
    // - You can load multiple fonts and use ImGui::PushFont()/PopFont() to select them.
    // - If a file cannot be loaded, AddFont functions will return a nullptr. Please handle those errors in your code (e.g. use an assertion, display an error and quit).
    // - Read 'docs/FONTS.md' for more instructions and details.
    // - Use '#define IMGUI_ENABLE_FREETYPE' in your imconfig file to use FreeType for higher quality font rendering.
    // - Remember that in C/C++ if you want to include a backslash \ in a string literal you need to write a double backslash \\ !
    //style.FontSizeBase = 20.0f;
    //io.Fonts->AddFontDefaultVector();
    //io.Fonts->AddFontDefaultBitmap();
    //io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\segoeui.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/DroidSans.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Roboto-Medium.ttf");
    //io.Fonts->AddFontFromFileTTF("../../misc/fonts/Cousine-Regular.ttf");
    //ImFont* font = io.Fonts->AddFontFromFileTTF("c:\\Windows\\Fonts\\ArialUni.ttf");
    //IM_ASSERT(font != nullptr);

    CreateModel();
    CreateTextures();
    CreateShaders();

    // Our state
    bool show_demo_window = false;
    bool show_debug_window_teleport = false;
    bool show_debug_window_edgefill = false;
    bool show_debug_window_surface_from_object = false;
    float show_debug_window_teleport_zoom = 1.0f;
    float show_debug_window_edgefill_zoom = 1.0f;
    float show_debug_window_surface_from_object_zoom = 1.0f;
    ImVec4 clear_color = ImVec4(0.45f, 0.55f, 0.60f, 1.00f);

    // Set up the game world.
    // 
    // Coordinate system is:
    // +X = right.
    // +Y = up.
    // +Z = forwards.
    // ...as the gods intended! :-)
    for ( int objNum = 0; objNum < g_NumObjects; objNum++ )
    {
        GameObject *object = &(g_GameObjects[objNum]);
        WorldOrientation newOrn;
        newOrn.posWorld.Init();
        Dir pos;
        pos.x = 2.0f * (float)(objNum & 0x1);
        pos.y = 1.0f * (float)(objNum & 0x2);
        pos.z = 2.0f * (float)(objNum >> 2);
        newOrn.posWorld.IncMeters ( pos );
        newOrn.worldFromObject = Rot::MakeRotateY ( PI * -0.5f );
        object->worldFromObject.FinishSimulationUpdate ( g_TimeStampGameClockSim, newOrn, true );

        object->speed = 1.0f + (((objNum & 0x8) == 0) ? 0.0f : 1.0f);
        if ((objNum == 0) ||
            (objNum == 3) ||
            (objNum == 12) ||
            (objNum == 15))
        {
            // Corners don't move, to give you a frame of reference.
            object->speed = 0.0f;
        }
        object->spin.x = (((objNum & 0x1) == 0) ? 0.0f : 1.0f);
        object->spin.y = (((objNum & 0x2) == 0) ? 0.0f : 1.0f);
        object->spin.z = (((objNum & 0x4) == 0) ? 0.0f : 1.0f);
    }

    g_worldFromShape.worldFromObject = Rot::identity;
    g_worldFromShape.posWorld.Init();

    g_WorldFromCamera.posWorld.Init();
    g_MayaFocusDistance = 8.0f;
    g_CameraMayaFocusPos = g_WorldFromCamera.posWorld;
    g_WorldFromCamera.posWorld.IncMeters ( Dir ( g_MayaFocusDistance * 0.2f, g_MayaFocusDistance * 0.2f, -g_MayaFocusDistance ) );
    g_WorldFromCamera.worldFromObject = Rot::identity;
    bool updateMayaOrbitCamPos = true;

    g_ConstantBufferData.DeltaUVStep = 0.1f;
    g_ConstantBufferData.StepSize = 0.02f;
    g_ConstantBufferData.StepScale = 10.0f;
    g_ConstantBufferData.ShadowAcneScaler = 0.01f;

    g_ConstantBufferData.SurfaceFromObjectTextureSize = (float)g_SurfaceFromObjectTextureSize;
    g_ConstantBufferData.HeightScale = 1.0f;
    g_ConstantBufferData.HeightOffset = 0.0f;
    g_ConstantBufferData.HeightNormalsScale = 1.5f;
    g_ConstantBufferData.HeightExtraMeshExtrude = 0.25f; // the default tessellation of 8 segments needs a little extra space or the rasteriser misses some pixels!

    g_ConstantBufferData.DebugMode = 0;
    g_ConstantBufferData.LightingMode = 4;
    g_ConstantBufferData.DistortionMode = 1;
    g_ConstantBufferData.DebugIterationsAfterTeleport = 0;
    g_ConstantBufferData.MaxSteps = -1;
    g_ConstantBufferData.DebugDampingFactor1 = 1.0f;
    g_ConstantBufferData.DebugDampingFactor2 = 0.0f;
    g_ConstantBufferData.DebugDampingFactor3 = 1.5f;
    g_ConstantBufferData.IndirectLighting = 0.2f;

    g_AlphaMode = 0;


    // Set up a simple "skeleton" to animate things with.
    g_boneFromObject[0] = Orn ( Rot::identity, Dir::zero );
    g_boneFromObject[1] = Orn ( Rot::identity, Dir::zero );
    g_boneFromObject[2] = Orn ( Rot::identity, Dir::zero );
    g_boneFromObject[3] = Orn ( Rot::identity, Dir::zero );



    // Main loop
    bool done = false;
    while (!done)
    {
        // --- Poll and handle messages (inputs, window resize, etc.)

        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, nullptr, 0U, 0U, PM_REMOVE))
        {
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
            if (msg.message == WM_QUIT)
                done = true;
        }
        if (done)
            break;

        // --- Handle window changes

        // Handle window being minimized or screen locked
        if (g_SwapChainOccluded && g_pSwapChain->Present(0, DXGI_PRESENT_TEST) == DXGI_STATUS_OCCLUDED)
        {
            ::Sleep(100); // Don't steal the user's power or CPU or GPU - they're doing something else.
            continue;
        }
        g_SwapChainOccluded = false;

        // Handle window resize (we don't resize directly in the WM_SIZE handler)
        if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
        {
            CleanupRenderTarget();
            g_pSwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
            g_ResizeWidth = g_ResizeHeight = 0;
            CreateRenderTarget();
        }

        // --- handle resource changes.

        // The steps should be the size of a texel, but for larger maps it needs to be slightly larger to smooth the filtering a bit.
        D3D11_TEXTURE2D_DESC desc;
        g_TextureHeight->GetDesc ( &desc );
        g_ConstantBufferData.DeltaUVStep = 1.0f / Min ( 512.0f, (float)desc.Width );

        g_SurfaceFromObjectTextureSize = 1 << g_SurfaceFromObjectTextureSizePow2;
        g_ConstantBufferData.SurfaceFromObjectTextureSize = (float)g_SurfaceFromObjectTextureSize;
        if ( ( g_TextureSet != g_TextureSet_Current ) ||
             ( g_SurfaceFromObjectTextureSize != g_SurfaceFromObjectTextureSize_Current ) )
        {
            CreateTextures();
        }

        if ( ( g_FloodFillTeleportEdgefill_Current != g_FloodFillTeleportEdgefill ) ||
             ( g_MeshNumber_Current != g_MeshNumber ) ||
             ( g_NumSegmentsAround_Current != g_NumSegmentsAround ) || 
             ( g_NumSegmentsLong_Current != g_NumSegmentsLong ) ||
             ( g_MiddleTubeLength_Current != g_MiddleTubeLength ) ||
             ( g_MeshRadius_Current != g_MeshRadius ) ||
             ( g_SurfaceThickness_Current != g_SurfaceThickness ) )
        {
            CreateModel();
            // Changing model means we need to regen the teleport and edgefill.
            CreateTeleportEdgefill();
        }

        if ( ( g_TextureTeleportMap == nullptr ) ||
             ( g_TextureEdgefillMap == nullptr ) )
        {
            CreateTeleportEdgefill();
        }

        // --- Handle time

        float secondsSinceLastFrame = g_TimeStampWallClock.UpdateRealTime();
        if (!g_GamePaused)
        {
            // Progress the in-game-time *rendered* clock
            g_TimeStampGameClockRender.AddSeconds ( secondsSinceLastFrame );
            // ...and catch the simulation clock up as close as it can get.
            CatchUpSimulationTime ( g_TimeStampGameClockRender );
        }

        // Hack some simple sun animation.
        if ( !g_sunAnimPaused )
        {
            g_sunAnimSeconds += secondsSinceLastFrame;
            if ( g_sunAnimSeconds > g_sunAnimPeriod )
            {
                g_sunAnimSeconds -= g_sunAnimPeriod;
            }
        }
        float sunAnimAngle = 2.0f * PI * g_sunAnimSeconds / g_sunAnimPeriod;
        Dir sunDirInWorld = Dir ( ( 1.0f - g_sunAnimHeight ) * sinf ( sunAnimAngle ),
                                  g_sunAnimHeight,
                                  ( 1.0f - g_sunAnimHeight ) * cosf ( sunAnimAngle )
                                ).GetNormalise();

        // Hack some simple skeleton animation.
        float g_boneAnimPeriod2 = g_boneAnimPeriod * 0.763f;
        if ( !g_boneAnimPaused )
        {
            g_boneAnimSeconds += secondsSinceLastFrame;
            if ( g_boneAnimSeconds > g_boneAnimPeriod )
            {
                g_boneAnimSeconds -= g_boneAnimPeriod;
            }

            g_boneAnimSeconds2 += secondsSinceLastFrame;
            if ( g_boneAnimSeconds2 > g_boneAnimPeriod2 )
            {
                g_boneAnimSeconds2 -= g_boneAnimPeriod2;
            }
        }
        float boneAnimAmount = g_boneAnimAmount * sinf ( 2 * PI * g_boneAnimSeconds / g_boneAnimPeriod );
        float boneAnimAmount2 = g_boneAnimAmount * sinf ( 2 * PI * g_boneAnimSeconds2 / g_boneAnimPeriod2 );

        switch ( g_MeshNumber )
        {
        case MeshType_Cube:
        case MeshType_CubePinched:
        {
            // Bone 0 is the "base" - it doesn't move.
            // Bone 1 flexes the +ve vertices along the X axis.
            // Bone 2 and 3 flex along -x and the z axis.
            Dir boneUpVector[4];
            Dir boneRightVector ( 0.0f, 0.0f, 1.0f );
            boneUpVector[0] = Dir ( 0.0f, 1.0f, 0.0f );
            boneUpVector[1] = Dir ( boneAnimAmount, 1.0f, boneAnimAmount2 );
            boneUpVector[2] = Dir ( -boneAnimAmount, 1.0f, 0.0f );
            boneUpVector[3] = Dir ( -boneAnimAmount2, 1.0f, -boneAnimAmount );

            for ( int boneNum = 0; boneNum < 4; boneNum++ )
            {
                Dir upVector = boneUpVector[boneNum].GetNormalise();
                Dir rightVector = upVector.Cross ( boneRightVector ).GetNormalise();
                Dir fwdVector = rightVector.Cross ( upVector ).GetNormalise();
                g_boneFromObject[boneNum] = Orn ( Rot::MakeFromBasis ( rightVector, upVector, fwdVector ), Dir::zero );
            }
        }
        break;
        case MeshType_Tube:
        case MeshType_TubePinched:
        {
#if 1
            // Bones are sequential and each offset along the previous one.
            Dir boneForwardVector[4];
            Dir bonePosOffset[4];
            Dir bonePosTranslate[4];
            Dir boneUpVector ( 0.0f, 1.0f, 0.0f );
            boneForwardVector[0]    = Dir ( 1.0f, 0.0f, 0.0f ); // sideways = more visible.
            bonePosOffset[0]        = Dir ( -0.5f * g_MiddleTubeLength, 0.0f, 0.0f ); // to center the thing better.
            bonePosTranslate[0]     = Dir ( 0.0f, 0.0f, 0.0f );
            boneForwardVector[1]    = Dir ( boneAnimAmount, boneAnimAmount2, 1.0f );
            bonePosOffset[1]        = Dir ( 0.0f, 0.0f, g_MiddleTubeLength * 0.15f );
            bonePosTranslate[1]     = Dir ( 0.0f, 0.0f, 0.0f );
            boneForwardVector[2]    = Dir ( 0.0f, boneAnimAmount, 1.0f );
            bonePosOffset[2]        = Dir ( 0.0f, 0.0f, g_MiddleTubeLength * 0.5f );
            bonePosTranslate[2]     = Dir ( 0.0f, 0.0f, 0.0f );
            boneForwardVector[3]    = Dir ( boneAnimAmount2, boneAnimAmount, 1.0f );
            bonePosOffset[3]        = Dir ( 0.0f, 0.0f, g_MiddleTubeLength * 0.85f );
            bonePosTranslate[3]     = Dir ( 0.0f, 0.0f, 0.0f );
#else
            // Bones are sequential and each offset along the previous one.
            Dir boneForwardVector[4];
            Dir bonePosOffset[4];
            Dir bonePosTranslate[4];
            Dir boneUpVector ( 0.0f, 1.0f, 0.0f );
            boneForwardVector[0]    = Dir ( 1.0f, 0.0f, 0.0f ); // sideways = more visible.
            bonePosOffset[0]        = Dir ( -0.5f * g_MiddleTubeLength, 0.0f, 0.0f ); // to center the thing better.
            bonePosTranslate[0]     = Dir ( 0.0f, 0.0f, 0.0f );
            boneForwardVector[1]    = Dir ( 0.0f, 0.0f, 1.0f );
            bonePosOffset[1]        = Dir ( 0.0f, 0.0f, g_MiddleTubeLength * 0.15f );
            bonePosTranslate[1]     = Dir ( 0.0f, 0.0f, g_MiddleTubeLength * 0.10f * boneAnimAmount );
            boneForwardVector[2]    = Dir ( 0.0f, 0.0f, 1.0f );
            bonePosOffset[2]        = Dir ( 0.0f, 0.0f, g_MiddleTubeLength * 0.5f );
            bonePosTranslate[2]     = Dir ( 0.0f, 0.0f, g_MiddleTubeLength * 0.10f * boneAnimAmount2 );
            boneForwardVector[3]    = Dir ( 0.0f, 0.0f, 1.0f );
            bonePosOffset[3]        = Dir ( 0.0f, 0.0f, g_MiddleTubeLength * 0.85f );
            bonePosTranslate[3]     = Dir ( 0.0f, 0.0f, g_MiddleTubeLength * 0.10f * boneAnimAmount );
#endif

            for ( int boneNum = 0; boneNum < 4; boneNum++ )
            {
                Dir fwdVector = boneForwardVector[boneNum].GetNormalise();
                Dir rightVector = boneUpVector.Cross ( fwdVector ).GetNormalise();
                Dir upVector = fwdVector.Cross ( rightVector ).GetNormalise();
                Dir rotateOrigin = bonePosOffset[boneNum];
                Orn boneFromObject = Orn ( Rot::MakeFromBasis ( rightVector, upVector, fwdVector ), bonePosTranslate[boneNum] );
                if ( boneNum > 0 )
                {
                    Orn postShift = Orn ( Rot::identity, rotateOrigin );
                    Orn preShift = Orn ( Rot::identity, -rotateOrigin );
                    g_boneFromObject[boneNum] = g_boneFromObject[boneNum-1] * postShift * boneFromObject * preShift;
                }
                else
                {
                    g_boneFromObject[boneNum] = boneFromObject;
                }
            }
        }
        break;
        }

        // Even when the "game" is paused, allow the camera to move.

        if (io.WantCaptureMouse)
        {
            // ImGui is processing something like a click or drag on its windows - ignore the state of the mouse.
        }
        else
        {
            // Alt key gives approximately Maya orbit controls:
            // Alt + LMB = orbit focus point.
            // Alt + MMB = pan focus point.
            // Alt + RMB = zoom in/out of focus point.
            //
            // Without Alt, but with LMB you get an FPS camera:
            // LMB = rotate camera.
            // LMB + RMB = pan up/down, roll left/right
            // LMB + WASD = pan left/right and fly forwards/back.

            if (io.KeyAlt)
            {
                if ((io.MouseDelta.x != 0) || (io.MouseDelta.y != 0))
                {
                    // Maya controls. Everything moves cameraMayaFocusPos, and then the camera position is derived from it.

                    if (io.MouseDown[ImGuiMouseButton_Left])
                    {
                        // Orbit focus. This is an heading/elevation deal, not a trackball.
                        updateMayaOrbitCamPos = true;
                        // Tune constant according to feel.
                        float radiansPerMouse = 0.003f; // TODO - put in ImGgui window
                        // Where does the camera's Z axis point in world space?
                        Dir basisZ = g_WorldFromCamera.worldFromObject.GetBasisZ();
                        float heading = atan2f ( basisZ.x, basisZ.z );
                        float pitch = asinf ( basisZ.y );
                        heading += radiansPerMouse * io.MouseDelta.x;
                        pitch += -radiansPerMouse * io.MouseDelta.y;
                        pitch = Clamp ( pitch, PI * -0.49f, PI * 0.49f ); // If the pitch hits 90 degrees, the heading is arbitrary and the camera goes nuts.
                        Rot cameraFromWorld = Rot::MakeRotateX ( -pitch ) * Rot::MakeRotateY ( heading );
                        g_WorldFromCamera.worldFromObject = cameraFromWorld.GetTranspose();
                    }
                    else if (io.MouseDown[ImGuiMouseButton_Right])
                    {
                        // Zoom in/out of focus position.
                        g_MayaFocusDistance *= powf ( 0.99f, io.MouseDelta.y ); // TODO - put constant in ImGgui window
                        updateMayaOrbitCamPos = true;
                    }
                    else if (io.MouseDown[ImGuiMouseButton_Middle])
                    {
                        // Pan focus around.
                        float moveSpeed = g_MayaFocusDistance * 0.001f; // TODO - put in ImGgui window
                        Dir moveCamera ( moveSpeed * -io.MouseDelta.x, moveSpeed * io.MouseDelta.y, 0.0f );
                        Dir moveWorld = g_WorldFromCamera.worldFromObject * moveCamera;
                        g_CameraMayaFocusPos.IncMeters ( moveWorld );
                        updateMayaOrbitCamPos = true;
                    }

                }
            }
            else if (io.MouseDown[ImGuiMouseButton_Left])
            {
                // WASD-style flying camera.
                if ((io.MouseDelta.x != 0) || (io.MouseDelta.y != 0))
                {
                    if (io.MouseDown[ImGuiMouseButton_Right])
                    {
                        // LMB + RMB = pan up/down, roll left/right
                        float radiansPerMouse = 0.01f; // TODO - put in ImGgui window
                        float moveSpeed = 0.01f; // TODO - put in ImGgui window
                        float rollZ = radiansPerMouse * io.MouseDelta.x;
                        // Note the multiplication order! The rotation is *in* camera space, so it's on the right side.
                        g_WorldFromCamera.worldFromObject = g_WorldFromCamera.worldFromObject * Rot::MakeRotateZ ( rollZ );
                        Dir moveWorld = g_WorldFromCamera.worldFromObject * Dir ( 0.0f, moveSpeed * -io.MouseDelta.y, 0.0f );
                        g_WorldFromCamera.posWorld.IncMeters ( moveWorld );
                    }
                    else
                    {
                        // LMB = rotate camera (not orbit).
                        float radiansPerMouse = 0.001f; // TODO - put in ImGgui window
                        float pitchX = radiansPerMouse * io.MouseDelta.y;
                        float yawY = radiansPerMouse * io.MouseDelta.x;
                        // Note the multiplication order! The rotations are *in* camera space, so they're on the right side.
                        g_WorldFromCamera.worldFromObject = g_WorldFromCamera.worldFromObject * Rot::MakeRotateX ( -pitchX ) * Rot::MakeRotateY ( -yawY );
                    }
                }

                // LMB + WASD = pan left/right and fly forwards/back.
                float movePerKey = secondsSinceLastFrame * 5.0f; // TODO - put in ImGgui window
                Dir translate ( Dir::zero );
                if (io.KeysData[ImGuiKey_W - ImGuiKey_NamedKey_BEGIN].Down)
                {
                    translate.z += movePerKey;
                }
                if (io.KeysData[ImGuiKey_S - ImGuiKey_NamedKey_BEGIN].Down)
                {
                    translate.z -= movePerKey;
                }
                if (io.KeysData[ImGuiKey_A - ImGuiKey_NamedKey_BEGIN].Down)
                {
                    translate.x -= movePerKey;
                }
                if (io.KeysData[ImGuiKey_D - ImGuiKey_NamedKey_BEGIN].Down)
                {
                    translate.x += movePerKey;
                }
                Dir translateWorld = g_WorldFromCamera.worldFromObject * translate;
                g_WorldFromCamera.posWorld.IncMeters ( translateWorld );
            }

            if (updateMayaOrbitCamPos)
            {
                Dir focusInWorld = g_WorldFromCamera.worldFromObject * Dir ( 0.0f, 0.0f, -g_MayaFocusDistance );
                g_WorldFromCamera.posWorld = g_CameraMayaFocusPos.AddMeters ( focusInWorld );
                updateMayaOrbitCamPos = false;
            }
        }


        // --- Start the Dear ImGui frame

        ImGui_ImplDX11_NewFrame();
        ImGui_ImplWin32_NewFrame();
        ImGui::NewFrame(); // Note - also handles mouse+keyboard, so don't gate by g_showDearImgui!
        if (g_showDearImgui)
        {
            // 1. Show the big demo window (Most of the sample code is in ImGui::ShowDemoWindow()! You can browse its code to learn more about Dear ImGui!).
            if (show_demo_window)
            {
                ImGui::ShowDemoWindow(&show_demo_window);
            }

            // 2. Show a simple window that we create ourselves. We use a Begin/End pair to create a named window.
            {
                ImGui::Begin("Skinned Heightfield");

                ImGui::SliderInt("Debug mode", &g_ConstantBufferData.DebugMode, 0, ARRAYSIZE(DebugModeNames) - 1, DebugModeNames[g_ConstantBufferData.DebugMode] );
                ImGui::SliderInt("Lighting mode", &g_ConstantBufferData.LightingMode, 0, ARRAYSIZE(LightingModeNames) - 1, LightingModeNames[g_ConstantBufferData.LightingMode]);
                ImGui::SliderInt("Distortion mode", &g_ConstantBufferData.DistortionMode, 0, ARRAYSIZE(DistortionModeNames) - 1, DistortionModeNames[g_ConstantBufferData.DistortionMode]);
                // Different alpha modes don't do much yet.
                //ImGui::SliderInt("Tranparency mode", &g_AlphaMode, 0, ARRAYSIZE(AlphaModeNames) - 1, AlphaModeNames[g_AlphaMode]);

                ImGui::SeparatorText("WIREFRAME:");
                ImGui::SliderInt("Mode", &g_WireframeMode, 0, ARRAYSIZE(WireframeModeNames) - 1, WireframeModeNames[g_WireframeMode]);
                ImGui::SliderFloat("Normals", &g_WireframeNormalScale, 0.0f, 1.0f);
                ImGui::SliderFloat("Tangents", &g_WireframeTangentScale, 0.0f, 1.0f);
                ImGui::SliderInt("Cyan triangle", &g_WireframeDebugTriangle[0], -1, g_NumTris-1);
                ImGui::SliderInt("Yellow triangle", &g_WireframeDebugTriangle[1], -1, g_NumTris-1);
                ImGui::Checkbox("Show unique mapping", &g_WireframeShowUniqueMapping);
                ImGui::SliderFloat("Unique mapping scale", &g_WireframeScale, 0.1f, 2.0f);
                ImGui::SliderFloat("Unique mapping X", &g_WireframeXOffset, -2.0f, 2.0f);
                ImGui::SliderFloat("Unique mapping Y", &g_WireframeYOffset, -2.0f, 2.0f);

                ImGui::SeparatorText("ANIMATION:");
                ImGui::Checkbox("Anim paused", &g_boneAnimPaused);
                ImGui::SliderFloat("Anim period", &g_boneAnimPeriod, 0.1f, 20.0f);
                ImGui::SliderFloat("Anim amount", &g_boneAnimAmount, 0.0f, 5.0f);
                ImGui::Checkbox("Sun anim paused", &g_sunAnimPaused);
                ImGui::SliderFloat("Sun anim period", &g_sunAnimPeriod, 0.1f, 20.0f);
                ImGui::SliderFloat("Sun anim elevation", &g_sunAnimHeight, 0.0f, 1.0f);

                ImGui::SeparatorText("MESH and TEXTURE (changes will be slow):");
                ImGui::SliderInt("Texture Set", &g_TextureSet, 0, 2);
                ImGui::SliderInt("Mesh", &g_MeshNumber, 0, ARRAYSIZE(MeshNames) - 1, MeshNames[g_MeshNumber]);
                ImGui::SliderInt("Num Segments Long", &g_NumSegmentsLong, 4, 64);
                ImGui::SliderInt("Num Segments Around", &g_NumSegmentsAround, 4, 32);
                ImGui::SliderFloat("Mesh length", &g_MiddleTubeLength, 0.1f, 10.0f);
                ImGui::SliderFloat("Mesh radius", &g_MeshRadius, 0.1f, 10.0f);
                ImGui::SliderFloat("Surface thickness", &g_SurfaceThickness, 0.1f, 10.0f);
                ImGui::SliderInt("SurfaceFromObject size pow2", &g_SurfaceFromObjectTextureSizePow2, 4, 12);

                ImGui::SeparatorText("RAYMARCHER:");
                ImGui::SliderFloat("Step size", &g_ConstantBufferData.StepSize, 0.001f, 0.05f);
                ImGui::SliderFloat("Step scale", &g_ConstantBufferData.StepScale, 0.0f, 100.0f);
                ImGui::SliderFloat("Heightfield scale", &g_ConstantBufferData.HeightScale, 0.0f, 2.5f);
                ImGui::SliderFloat("Heightfield offset", &g_ConstantBufferData.HeightOffset, -1.0f, 1.0f);
                ImGui::SliderFloat("Mesh extra extrusion", &g_ConstantBufferData.HeightExtraMeshExtrude, 0.0f, 1.0f);
                ImGui::SliderInt("Steps after teleport", &g_ConstantBufferData.DebugIterationsAfterTeleport, 0, 10 );
                ImGui::SliderInt("Max steps (-1 = off)", &g_ConstantBufferData.MaxSteps, -1, 100 );
                ImGui::SliderFloat("Damping factor 1", &g_ConstantBufferData.DebugDampingFactor1, 0.0f, 5.0f );
                ImGui::SliderFloat("Damping factor 2", &g_ConstantBufferData.DebugDampingFactor2, -10.0f, 1.0f );
                ImGui::SliderFloat("Damping factor 3", &g_ConstantBufferData.DebugDampingFactor3, 1.0f, 5.0f );

                ImGui::SeparatorText("LIGHTING AND SHADOWS:");
                ImGui::SliderFloat("Indirect lighting", &g_ConstantBufferData.IndirectLighting, 0.0f, 1.0f);
                ImGui::SliderFloat("Shadow acne offset", &g_ConstantBufferData.ShadowAcneScaler, 0.0f, 0.1f);
                ImGui::SliderFloat("Height scale for normals", &g_ConstantBufferData.HeightNormalsScale, 0.0f, 2.0f);
                ImGui::SliderFloat("DeltaUVStep for height->normal map", &g_ConstantBufferData.DeltaUVStep, 0.0f, 0.01f);

                ImGui::SeparatorText("WINDOWS:");
                ImGui::Checkbox("Show Teleport Map", &show_debug_window_teleport);
                ImGui::Checkbox("Show Edgefill Map", &show_debug_window_edgefill);
                ImGui::Checkbox("Floodfill teleport and edgefill", &g_FloodFillTeleportEdgefill);
                ImGui::Checkbox("Show SurfaceFromObject Map", &show_debug_window_surface_from_object);
                ImGui::Checkbox("Intro/controls Window", &g_showIntroWindow);
                //ImGui::Checkbox("Dear ImGui Demo Window", &show_demo_window);

                ImGui::SeparatorText("MISC CONTROLS:");
                ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
                ImGui::Checkbox("Vsync", &g_VsyncEnabled);
                //ImGui::Checkbox("Paused", &g_GamePaused);
                //ImGui::SliderFloat("Near clip plane", &g_nearClipPlane, 0.001f, 1.0f);
                //ImGui::SliderFloat("Simulation time step (secs)", &g_SimulationTimeStepSeconds, 0.01f, 1.0f);
                ImGui::ColorEdit3("clear color", (float*)&clear_color); // Edit 3 floats representing a color

                ImGui::End();
            }

            if (show_debug_window_teleport)
            {
                ImGui::Begin("TextureTeleportMap", &show_debug_window_teleport, ImGuiWindowFlags_HorizontalScrollbar );
                ImGui::SliderFloat("Zoom", &show_debug_window_teleport_zoom, 0.1f, 10.0f);
                ImGui::Image((ImTextureID)(intptr_t)g_TextureTeleportMapSRV, ImVec2(show_debug_window_teleport_zoom * (float)g_SurfaceFromObjectTextureSize, show_debug_window_teleport_zoom * (float)g_SurfaceFromObjectTextureSize));
                ImGui::End();
            }

            if (show_debug_window_edgefill)
            {
                ImGui::Begin("TextureEdgefillMap", &show_debug_window_edgefill, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::SliderFloat("Zoom", &show_debug_window_edgefill_zoom, 0.1f, 10.0f);
                ImGui::Image((ImTextureID)(intptr_t)g_TextureEdgefillMapSRV, ImVec2(show_debug_window_edgefill_zoom * (float)g_SurfaceFromObjectTextureSize, show_debug_window_edgefill_zoom * (float)g_SurfaceFromObjectTextureSize));
                ImGui::End();
            }

            if (show_debug_window_surface_from_object)
            {
                ImGui::Begin("SurfaceFromObjectMap", &show_debug_window_surface_from_object, ImGuiWindowFlags_HorizontalScrollbar);
                ImGui::SliderFloat("Zoom", &show_debug_window_surface_from_object_zoom, 0.1f, 10.0f);
                ImGui::Image((ImTextureID)(intptr_t)g_TextureSurfaceFromObject0SRV, ImVec2(show_debug_window_surface_from_object_zoom * (float)g_SurfaceFromObjectTextureSize, show_debug_window_surface_from_object_zoom * (float)g_SurfaceFromObjectTextureSize));
                ImGui::Image((ImTextureID)(intptr_t)g_TextureSurfaceFromObject1SRV, ImVec2(show_debug_window_surface_from_object_zoom * (float)g_SurfaceFromObjectTextureSize, show_debug_window_surface_from_object_zoom * (float)g_SurfaceFromObjectTextureSize));
                ImGui::Image((ImTextureID)(intptr_t)g_TextureSurfaceFromObject2SRV, ImVec2(show_debug_window_surface_from_object_zoom * (float)g_SurfaceFromObjectTextureSize, show_debug_window_surface_from_object_zoom * (float)g_SurfaceFromObjectTextureSize));
                ImGui::Image((ImTextureID)(intptr_t)g_TextureSurfaceFromObject3SRV, ImVec2(show_debug_window_surface_from_object_zoom * (float)g_SurfaceFromObjectTextureSize, show_debug_window_surface_from_object_zoom * (float)g_SurfaceFromObjectTextureSize));
                ImGui::End();
            }

            if (g_showIntroWindow)
            {
                ImGui::Begin("Welcome to the DASHR demo", &g_showIntroWindow);
                ImGui::Text("Controls:");
                ImGui::Text("Alt+LMB: orbit focus");
                ImGui::Text("Alt+RMB: towards/away from focus");
                ImGui::Text("Alt+MMB: pan focus");
                ImGui::Text("LMB: FPS look");
                ImGui::Text("LMB + WASD: FPS move");
                ImGui::Text("LMB+RMB: FPS rotate + move up/down");
                ImGui::Text("Esc: hide/show windows");
                ImGui::Text("Space: animation on/off");
                ImGui::End();
            }
        }
        // Rendering
        ImGui::Render();

        // --- Set up the various matrices.

        float fieldOfViewInRadians = 3.1415f * 0.25f;
        float aspectRatio = (float)g_CurrentWidth / (float)g_CurrentHeight;

        // Note this projection matrix has X=right, Y=up and Z=backwards.
        // Which directions are "right" is a long-standing argument in the graphics industry. There are many correct answers - it's a good idea to be adaptable!
        // 
        // Note - do NOT combine your projection and world matrices together!
        // Mathematically this is a valid thing to do, but in practice it
        // can lead to floating-point cancellation and bad precision problems.
        // A possible performance improvement is to notice that there are many 0s and 1s
        // in the projection matrix, and do the maths explciitly.
        // 
        // This uses the "reverze Z" and "infinite far clip plane", both of which
        // are the right defaults to use with modern float32 Z buffers.

        float invTanHalfFovV = 1.0f / tanf ( fieldOfViewInRadians * 0.5f );
        float invTanHalfFovH = invTanHalfFovV / aspectRatio;
        g_ConstantBufferData.projectionFromCameraMatrix = ProjectionMatrixInfFarClipReverseZDirect3D ( invTanHalfFovH, invTanHalfFovV, g_nearClipPlane, false );

        // Set up standard object transform.
        WorldOrientation worldFromObject = g_worldFromShape;
        Orn cameraFromObject = g_WorldFromCamera.GetFrom ( worldFromObject );
        Mat44 cameraFromObjectMatrix = cameraFromObject.ToMat44();
        g_ConstantBufferData.cameraFromObjectMatrix = cameraFromObjectMatrix;
        g_ConstantBufferData.objectFromCameraMatrix = cameraFromObjectMatrix.GetInverse();

        Rot objectFromWorld = worldFromObject.worldFromObject.GetTranspose();
        g_ConstantBufferData.SunDirInObject = (objectFromWorld * sunDirInWorld).GetNormalise();

        // Transfer bones.
        for ( int boneNum = 0; boneNum < 4; boneNum++ )
        {
            g_ConstantBufferData.BoneFromObject[boneNum] = g_boneFromObject[boneNum].ToMat44();
        }

        // --- Clear the buffers.

        const float clear_color_with_alpha[4] = { clear_color.x * clear_color.w, clear_color.y * clear_color.w, clear_color.z * clear_color.w, clear_color.w };
        g_pd3dDeviceContext->ClearRenderTargetView(g_mainRenderTargetView, clear_color_with_alpha);
        g_pd3dDeviceContext->ClearDepthStencilView(g_DepthStencilView, D3D11_CLEAR_DEPTH, 0.0f, 0); // reverze Z puts the far plane at 0.0f

        // Don't technically NEED to clear these, since we're going to render all the relevant parts every frame anyway.
        const float zero[4] = { 0.0f, 0.0f, 0.0f, 1.0f };
        g_pd3dDeviceContext->ClearRenderTargetView(g_TextureSurfaceFromObjectTemp0RTV, zero);
        g_pd3dDeviceContext->ClearRenderTargetView(g_TextureSurfaceFromObjectTemp1RTV, zero);
        g_pd3dDeviceContext->ClearRenderTargetView(g_TextureSurfaceFromObjectTemp2RTV, zero);
        g_pd3dDeviceContext->ClearRenderTargetView(g_TextureSurfaceFromObjectTemp3RTV, zero);
        g_pd3dDeviceContext->ClearRenderTargetView(g_TextureSurfaceFromObject0RTV, zero);
        g_pd3dDeviceContext->ClearRenderTargetView(g_TextureSurfaceFromObject1RTV, zero);
        g_pd3dDeviceContext->ClearRenderTargetView(g_TextureSurfaceFromObject2RTV, zero);
        g_pd3dDeviceContext->ClearRenderTargetView(g_TextureSurfaceFromObject3RTV, zero);

        // --- Render the scene

        // Render object-to-surface mapping matrices into the textures.
        // This will also set up the constant buffer.
        RenderDistortionPass();


        // Edgefill pass. This will copy parts of the distortion to places the mesh
        // didn't actually reach. This means the ray can walk off the edge of the
        // mesh in UV space and not get garbage values.

        {
            PipelineState const *curPipeline = &(g_Pipeline[PipelineState::Pipeline_Edgefill]);

            const D3D11_VIEWPORT viewport = CD3D11_VIEWPORT(0.0f, 0.0f, (float)g_SurfaceFromObjectTextureSize, (float)g_SurfaceFromObjectTextureSize);
            g_pd3dDeviceContext->RSSetViewports(1, &viewport);

            // Subtley - need to set new RTs to unbind the previous RTs before you set them as textures!
            ID3D11RenderTargetView *rtvs[4] = {
                g_TextureSurfaceFromObject0RTV,
                g_TextureSurfaceFromObject1RTV,
                g_TextureSurfaceFromObject2RTV,
                g_TextureSurfaceFromObject3RTV,
            };
            g_pd3dDeviceContext->OMSetRenderTargets(4, rtvs, nullptr);

            g_pd3dDeviceContext->VSSetShader(curPipeline->vertexShader, nullptr, 0);
            g_pd3dDeviceContext->VSSetConstantBuffers(0, 1, &g_ConstantBuffer);

            g_pd3dDeviceContext->PSSetShader(curPipeline->pixelShader, nullptr, 0);
            g_pd3dDeviceContext->PSSetConstantBuffers( 0, 1, &g_ConstantBuffer);

            g_pd3dDeviceContext->PSSetShaderResources( 0, 1, &g_TextureEdgefillMapSRV );
            g_pd3dDeviceContext->PSSetShaderResources( 1, 1, &g_TextureSurfaceFromObjectTemp0SRV );
            g_pd3dDeviceContext->PSSetShaderResources( 2, 1, &g_TextureSurfaceFromObjectTemp1SRV );
            g_pd3dDeviceContext->PSSetShaderResources( 3, 1, &g_TextureSurfaceFromObjectTemp2SRV );
            g_pd3dDeviceContext->PSSetShaderResources( 4, 1, &g_TextureSurfaceFromObjectTemp3SRV );
            g_pd3dDeviceContext->PSSetSamplers (0, 1, &g_SamplerLinear);
            g_pd3dDeviceContext->PSSetSamplers (1, 1, &g_SamplerPoint);

            g_pd3dDeviceContext->OMSetDepthStencilState(g_DepthStencilStateOff, 0);
            Vec4 blendFactor ( 1.0f, 1.0f, 1.0f, 1.0f );
            g_pd3dDeviceContext->OMSetBlendState(g_BlendStateOff, blendFactor.AsFloatPtr(), ~0u);

            g_pd3dDeviceContext->RSSetState(g_RasterizerState);
            g_pd3dDeviceContext->IASetInputLayout(curPipeline->inputLayout);

            ID3D11Buffer* buffers[] = { g_VertexBufferEdgefill };
            const UINT stride[] = { sizeof(VertexBufferStruct) };
            const UINT offset[] = { 0 };

            g_pd3dDeviceContext->IASetVertexBuffers(0, 1, buffers, stride, offset);

            g_pd3dDeviceContext->IASetIndexBuffer(g_IndexBufferEdgefill, DXGI_FORMAT_R32_UINT, 0); // our indices are uint32
            g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            g_pd3dDeviceContext->DrawIndexed(6, 0, 0);
        }

        // Main pass.

        {
            PipelineState const *curPipeline = &(g_Pipeline[PipelineState::Pipeline_Main]);

            const D3D11_VIEWPORT viewport = CD3D11_VIEWPORT(0.0f, 0.0f, (float)g_CurrentWidth, (float)g_CurrentHeight);
            g_pd3dDeviceContext->RSSetViewports(1, &viewport);

            g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, g_DepthStencilView);

            g_pd3dDeviceContext->VSSetShader(curPipeline->vertexShader, nullptr, 0);
            g_pd3dDeviceContext->VSSetConstantBuffers(0, 1, &g_ConstantBuffer);

            g_pd3dDeviceContext->PSSetShader(curPipeline->pixelShader, nullptr, 0);
            g_pd3dDeviceContext->PSSetConstantBuffers( 0, 1, &g_ConstantBuffer);

            g_pd3dDeviceContext->PSSetShaderResources( 0, 1, &g_TextureAlbedoSRV );
            g_pd3dDeviceContext->PSSetShaderResources( 1, 1, &g_TextureHeightSRV );
            g_pd3dDeviceContext->PSSetShaderResources( 2, 1, &g_TextureNormalSRV );

            g_pd3dDeviceContext->PSSetShaderResources( 3, 1, &g_TextureSurfaceFromObject0SRV );
            g_pd3dDeviceContext->PSSetShaderResources( 4, 1, &g_TextureSurfaceFromObject1SRV );
            g_pd3dDeviceContext->PSSetShaderResources( 5, 1, &g_TextureSurfaceFromObject2SRV );
            g_pd3dDeviceContext->PSSetShaderResources( 6, 1, &g_TextureSurfaceFromObject3SRV );
            g_pd3dDeviceContext->PSSetShaderResources( 7, 1, &g_TextureTeleportMapSRV );
            g_pd3dDeviceContext->PSSetSamplers (0, 1, &g_SamplerLinear);

            g_pd3dDeviceContext->OMSetDepthStencilState(g_DepthStencilState, 0);
            Vec4 blendFactor ( 0.0f, 0.0f, 0.0f, 0.0f );
            switch ( g_AlphaMode )
            {
            case 0: // just alpha test
                g_pd3dDeviceContext->OMSetBlendState(g_BlendStateOff, blendFactor.AsFloatPtr(), ~0u); break;
                break;
            case 2: // alpha test + blend
                g_pd3dDeviceContext->OMSetBlendState(g_BlendStateAlpha, blendFactor.AsFloatPtr(), ~0u); break;
                break;
            case 1: // test + PS depth
            case 3: // alpha-to-coverage
            case 4: // alpha-to-coverage + PS depth
                // TODO;
                g_pd3dDeviceContext->OMSetBlendState(g_BlendStateOff, blendFactor.AsFloatPtr(), ~0u); break;
                break;
            default:
                ASSERT ( false );
                break;
            }
            g_pd3dDeviceContext->RSSetState(g_RasterizerState);
            g_pd3dDeviceContext->IASetInputLayout(curPipeline->inputLayout);

            ID3D11Buffer* buffers[] = { g_VertexBuffer };
            const UINT stride[] = { sizeof(VertexBufferStruct) };
            const UINT offset[] = { 0 };

            g_pd3dDeviceContext->IASetVertexBuffers(0, 1, buffers, stride, offset);

            g_pd3dDeviceContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0); // our indices are uint32
            g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

            g_pd3dDeviceContext->DrawIndexed((UINT)g_NumTris * 3, 0, 0);
        }

        // Wireframe pass
        if ( g_WireframeMode > 0 )
        {
            int numLineVerts = 0;

            // Animate the same way the shader does.
            for ( int vertNum = 0; vertNum < g_NumVerts; vertNum++ )
            {
                VertexBufferStruct vertex = g_VertexData[vertNum];

                Orn boneFromObjectTotal;
                boneFromObjectTotal                  = g_boneFromObject[0].ScalarMultiply ( vertex.BoneWeights.x );
                boneFromObjectTotal.ComponentwiseInc ( g_boneFromObject[1].ScalarMultiply ( vertex.BoneWeights.y ) );
                boneFromObjectTotal.ComponentwiseInc ( g_boneFromObject[2].ScalarMultiply ( vertex.BoneWeights.z ) );
                boneFromObjectTotal.ComponentwiseInc ( g_boneFromObject[3].ScalarMultiply ( vertex.BoneWeights.w ) );

                vertex.Pos       = (boneFromObjectTotal * vertex.Pos.ToPos()       ).ToVec3();
                vertex.Normal    = (boneFromObjectTotal * vertex.Normal.ToDir()    ).ToVec3();
                vertex.Tangent   = (boneFromObjectTotal * vertex.Tangent.ToDir()   ).ToVec3();
                vertex.Bitangent = (boneFromObjectTotal * vertex.Bitangent.ToDir() ).ToVec3();

                if ( g_WireframeMode < 3 )
                {
                    // Extrude the base mesh to the rendered one.
                    vertex.Pos += vertex.Normal * vertex.TexCoord.z * ( g_ConstantBufferData.HeightScale + g_ConstantBufferData.HeightExtraMeshExtrude ) * 0.5f;
                }

                g_VertexDataTemp[vertNum] = vertex;
            }

            Vec4 const colourSolid  ( 1.0f, 1.0f, 1.0f, 1.0f );
            Vec4 const colourRed    ( 1.0f, 0.0f, 0.0f, 1.0f );
            Vec4 const colourGreen  ( 0.0f, 1.0f, 0.0f, 1.0f );
            Vec4 const colourBlue   ( 0.0f, 0.0f, 1.0f, 1.0f );
            Vec4 const colourEdge   ( 1.0f, 0.0f, 1.0f, 1.0f );
            Vec4 const colourCyan   ( 0.0f, 1.0f, 1.0f, 1.0f );
            Vec4 const colourYellow ( 1.0f, 1.0f, 0.0f, 1.0f );

            UINT *curIndex = g_IndexData;
            for ( int triNum = 0; triNum < g_NumTris; triNum++ )
            {
                int index0 = *curIndex++;
                int index1 = *curIndex++;
                int index2 = *curIndex++;

                VertexBufferStruct *v0 = g_VertexDataTemp + index0;
                VertexBufferStruct *v1 = g_VertexDataTemp + index1;
                VertexBufferStruct *v2 = g_VertexDataTemp + index2;

                g_WireframeVerts[numLineVerts++] = { v0->Pos, colourSolid };
                g_WireframeVerts[numLineVerts++] = { v1->Pos, colourSolid };
                g_WireframeVerts[numLineVerts++] = { v1->Pos, colourSolid };
                g_WireframeVerts[numLineVerts++] = { v2->Pos, colourSolid };
                g_WireframeVerts[numLineVerts++] = { v2->Pos, colourSolid };
                g_WireframeVerts[numLineVerts++] = { v0->Pos, colourSolid };
            }

            for ( int edgeNum = 0; edgeNum < g_EdgeListCount; edgeNum++ )
            {
                int index0 = g_EdgeList[edgeNum].v1;
                int index1 = g_EdgeList[edgeNum].v2;

                VertexBufferStruct *v0 = g_VertexDataTemp + index0;
                VertexBufferStruct *v1 = g_VertexDataTemp + index1;

                g_WireframeVerts[numLineVerts++] = { v0->Pos, colourEdge };
                g_WireframeVerts[numLineVerts++] = { v1->Pos, colourEdge };
            }

            if ( g_WireframeMode == 2 )
            {
                // Also draw tangent space.
                for ( int vertNum = 0; vertNum < g_NumVerts; vertNum++ )
                {
                    VertexBufferStruct vertex = g_VertexDataTemp[vertNum];
                    g_WireframeVerts[numLineVerts++] = { vertex.Pos,                                              colourRed };
                    g_WireframeVerts[numLineVerts++] = { vertex.Pos + vertex.Tangent   * g_WireframeTangentScale, colourRed };
                    g_WireframeVerts[numLineVerts++] = { vertex.Pos,                                              colourGreen };
                    g_WireframeVerts[numLineVerts++] = { vertex.Pos + vertex.Bitangent * g_WireframeTangentScale, colourGreen };
                    g_WireframeVerts[numLineVerts++] = { vertex.Pos,                                              colourBlue };
                    g_WireframeVerts[numLineVerts++] = { vertex.Pos + vertex.Normal    * g_WireframeNormalScale,  colourBlue };
                }
            }

            for ( int triNum = 0; triNum < 2; triNum++ )
            {
                if ( g_WireframeDebugTriangle[triNum] >= 0 )
                {
                    if ( g_WireframeDebugTriangle[triNum] >= g_NumTris )
                    {
                        g_WireframeDebugTriangle[triNum] = g_NumTris - 1;
                    }

                    Vec4 triColour = colourCyan;
                    if ( triNum == 1 )
                    {
                        triColour = colourYellow;
                    }

                    int curTriNum = g_WireframeDebugTriangle[triNum];

                    curIndex = g_IndexData + curTriNum * 3;

                    int index0 = *curIndex++;
                    int index1 = *curIndex++;
                    int index2 = *curIndex++;

                    VertexBufferStruct *v0 = g_VertexDataTemp + index0;
                    VertexBufferStruct *v1 = g_VertexDataTemp + index1;
                    VertexBufferStruct *v2 = g_VertexDataTemp + index2;

                    static float animAmount = 0.0f;
                    animAmount += secondsSinceLastFrame * 0.5f;
                    animAmount -= floorf ( animAmount );
                    float animBlend = animAmount * 2.0f;
                    if ( animBlend > 1.0f )
                    {
                        animBlend = 2.0f - animBlend;
                    }
                    Vec3 center = ( v0->Pos + v1->Pos + v2->Pos ) * ( 1.0f / 3.0f );
                    float lerp = 1.0f - 0.2f * animBlend;

                    Vec3 pos0 = v0->Pos * lerp + center * ( 1.0f - lerp );
                    Vec3 pos1 = v1->Pos * lerp + center * ( 1.0f - lerp );
                    Vec3 pos2 = v2->Pos * lerp + center * ( 1.0f - lerp );

                    g_WireframeVerts[numLineVerts++] = { pos0, triColour };
                    g_WireframeVerts[numLineVerts++] = { pos1, triColour };
                    g_WireframeVerts[numLineVerts++] = { pos1, triColour };
                    g_WireframeVerts[numLineVerts++] = { pos2, triColour };
                    g_WireframeVerts[numLineVerts++] = { pos2, triColour };
                    g_WireframeVerts[numLineVerts++] = { pos0, triColour };
                }
            }

            ASSERT ( numLineVerts < ARRAYSIZE(g_WireframeVerts) );

            // Do we need to re-create the wireframe VB?
            if ( ( g_VertexBufferWireframeSize < numLineVerts ) || ( g_VertexBufferWireframe == nullptr ) )
            {
                SAFE_RELEASE(g_VertexBufferWireframe);
                int newNumVerts = numLineVerts * 2;
                g_VertexBufferWireframeSize = newNumVerts;

                D3D11_BUFFER_DESC bufferDesc = CD3D11_BUFFER_DESC((UINT)sizeof(g_WireframeVerts[0]) * g_VertexBufferWireframeSize, D3D11_BIND_VERTEX_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
                HRESULT hres = g_pd3dDevice->CreateBuffer(&bufferDesc, nullptr, &g_VertexBufferWireframe);
                ASSERT ( hres == S_OK );
            }

            if ( numLineVerts > 0 )
            {
                D3D11_MAPPED_SUBRESOURCE mappedSubRes;
                g_pd3dDeviceContext->Map(g_VertexBufferWireframe, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubRes);
                CopyMemory(mappedSubRes.pData, &g_WireframeVerts, sizeof(g_WireframeVerts[0]) * numLineVerts);
                g_pd3dDeviceContext->Unmap(g_VertexBufferWireframe, 0);

                PipelineState const *curPipeline = &(g_Pipeline[PipelineState::Pipeline_Wireframe]);

                const D3D11_VIEWPORT viewport = CD3D11_VIEWPORT(0.0f, 0.0f, (float)g_CurrentWidth, (float)g_CurrentHeight);
                g_pd3dDeviceContext->RSSetViewports(1, &viewport);

                g_pd3dDeviceContext->OMSetRenderTargets(1, &g_mainRenderTargetView, g_DepthStencilView);

                g_pd3dDeviceContext->VSSetShader(curPipeline->vertexShader, nullptr, 0);
                g_pd3dDeviceContext->VSSetConstantBuffers(0, 1, &g_ConstantBuffer);

                g_pd3dDeviceContext->PSSetShader(curPipeline->pixelShader, nullptr, 0);
                g_pd3dDeviceContext->PSSetConstantBuffers( 0, 1, &g_ConstantBuffer);

                if ( g_WireframeMode == 3 )
                {
                    g_pd3dDeviceContext->OMSetDepthStencilState(g_DepthStencilStateOff, 0);
                }
                else
                {
                    g_pd3dDeviceContext->OMSetDepthStencilState(g_DepthStencilState, 0);
                }

                Vec4 blendFactor ( 0.0f, 0.0f, 0.0f, 0.0f );
                g_pd3dDeviceContext->OMSetBlendState(g_BlendStateOff, blendFactor.AsFloatPtr(), ~0u);
                g_pd3dDeviceContext->RSSetState(g_RasterizerState);
                g_pd3dDeviceContext->IASetInputLayout(curPipeline->inputLayout);

                ID3D11Buffer* buffers[] = { g_VertexBufferWireframe };
                const UINT stride[] = { sizeof(VertexBufferWireframeStruct) };
                const UINT offset[] = { 0 };

                g_pd3dDeviceContext->IASetVertexBuffers(0, 1, buffers, stride, offset);

                // An unindexed line list.
                g_pd3dDeviceContext->IASetIndexBuffer(nullptr, DXGI_FORMAT_R32_UINT, 0);
                g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_LINELIST);

                g_pd3dDeviceContext->Draw(numLineVerts, 0);

                if ( g_WireframeShowUniqueMapping )
                {
                    // Grody little hack to put an isometric view in.
                    // The camera matrix will be reset next frame.
                    float scaleX = g_WireframeScale;
                    float scaleY = -g_WireframeScale * (float)g_CurrentWidth / (float)g_CurrentHeight;
                    Mat44 projectionFromCameraMatrix = Mat44::identity;
                    projectionFromCameraMatrix.x.x = scaleX;
                    projectionFromCameraMatrix.x.w = g_WireframeXOffset - 0.5 * scaleX;
                    projectionFromCameraMatrix.y.y = scaleY;
                    projectionFromCameraMatrix.y.w = g_WireframeYOffset - 0.5 * scaleY;
                    projectionFromCameraMatrix.z.z = 0.0f;
                    projectionFromCameraMatrix.w.z = 0.5f;
                    projectionFromCameraMatrix.w.w = 1.0f;
                    g_ConstantBufferData.cameraFromObjectMatrix = Mat44::identity;
                    g_ConstantBufferData.projectionFromCameraMatrix = projectionFromCameraMatrix;

                    g_pd3dDeviceContext->Map(g_ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubRes);
                    CopyMemory(mappedSubRes.pData, &g_ConstantBufferData, sizeof(g_ConstantBufferData));
                    g_pd3dDeviceContext->Unmap(g_ConstantBuffer, 0);

                    numLineVerts = 0;
                    curIndex = g_IndexData;
                    for ( int triNum = 0; triNum < g_NumTris; triNum++ )
                    {
                        int index0 = *curIndex++;
                        int index1 = *curIndex++;
                        int index2 = *curIndex++;

                        VertexBufferStruct *v0 = g_VertexDataTemp + index0;
                        VertexBufferStruct *v1 = g_VertexDataTemp + index1;
                        VertexBufferStruct *v2 = g_VertexDataTemp + index2;

                        Vec3 pos0 = Vec3 ( v0->TexCoord.x, v0->TexCoord.y, 0.0f );
                        Vec3 pos1 = Vec3 ( v1->TexCoord.x, v1->TexCoord.y, 0.0f );
                        Vec3 pos2 = Vec3 ( v2->TexCoord.x, v2->TexCoord.y, 0.0f );

                        g_WireframeVerts[numLineVerts++] = { pos0, colourSolid };
                        g_WireframeVerts[numLineVerts++] = { pos1, colourSolid };
                        g_WireframeVerts[numLineVerts++] = { pos1, colourSolid };
                        g_WireframeVerts[numLineVerts++] = { pos2, colourSolid };
                        g_WireframeVerts[numLineVerts++] = { pos2, colourSolid };
                        g_WireframeVerts[numLineVerts++] = { pos0, colourSolid };
                    }

                    for ( int edgeNum = 0; edgeNum < g_EdgeListCount; edgeNum++ )
                    {
                        // Need to draw both edges in the unique view, since they are noy coincident.
                        int index0 = g_EdgeList[edgeNum].v1;
                        int index1 = g_EdgeList[edgeNum].v2;
                        int index0P = g_EdgeList[edgeNum].v1Prox;
                        int index1P = g_EdgeList[edgeNum].v2Prox;

                        VertexBufferStruct *v0 = g_VertexDataTemp + index0;
                        VertexBufferStruct *v1 = g_VertexDataTemp + index1;
                        VertexBufferStruct *v0P = g_VertexDataTemp + index0P;
                        VertexBufferStruct *v1P = g_VertexDataTemp + index1P;

                        Vec3 pos0 = Vec3 ( v0->TexCoord.x, v0->TexCoord.y, 0.0f );
                        Vec3 pos1 = Vec3 ( v1->TexCoord.x, v1->TexCoord.y, 0.0f );
                        Vec3 pos0P = Vec3 ( v0P->TexCoord.x, v0P->TexCoord.y, 0.0f );
                        Vec3 pos1P = Vec3 ( v1P->TexCoord.x, v1P->TexCoord.y, 0.0f );

                        g_WireframeVerts[numLineVerts++] = { pos0, colourEdge };
                        g_WireframeVerts[numLineVerts++] = { pos1, colourEdge };
                        g_WireframeVerts[numLineVerts++] = { pos0P, colourEdge };
                        g_WireframeVerts[numLineVerts++] = { pos1P, colourEdge };
                    }

                    for ( int triNum = 0; triNum < 2; triNum++ )
                    {
                        if ( g_WireframeDebugTriangle[triNum] >= 0 )
                        {
                            if ( g_WireframeDebugTriangle[triNum] >= g_NumTris )
                            {
                                g_WireframeDebugTriangle[triNum] = g_NumTris - 1;
                            }

                            Vec4 triColour = colourCyan;
                            if ( triNum == 1 )
                            {
                                triColour = colourYellow;
                            }

                            int curTriNum = g_WireframeDebugTriangle[triNum];

                            curIndex = g_IndexData + curTriNum * 3;

                            int index0 = *curIndex++;
                            int index1 = *curIndex++;
                            int index2 = *curIndex++;

                            VertexBufferStruct *v0 = g_VertexDataTemp + index0;
                            VertexBufferStruct *v1 = g_VertexDataTemp + index1;
                            VertexBufferStruct *v2 = g_VertexDataTemp + index2;

                            Vec3 pos0 = Vec3 ( v0->TexCoord.x, v0->TexCoord.y, 0.0f );
                            Vec3 pos1 = Vec3 ( v1->TexCoord.x, v1->TexCoord.y, 0.0f );
                            Vec3 pos2 = Vec3 ( v2->TexCoord.x, v2->TexCoord.y, 0.0f );

                            static float animAmount = 0.0f;
                            animAmount += secondsSinceLastFrame * 0.5f;
                            animAmount -= floorf ( animAmount );
                            float animBlend = animAmount * 2.0f;
                            if ( animBlend > 1.0f )
                            {
                                animBlend = 2.0f - animBlend;
                            }
                            Vec3 center = ( pos0 + pos1 + pos2 ) * ( 1.0f / 3.0f );
                            float lerp = 1.0f - 0.2f * animBlend;

                            pos0 = pos0 * lerp + center * ( 1.0f - lerp );
                            pos1 = pos1 * lerp + center * ( 1.0f - lerp );
                            pos2 = pos2 * lerp + center * ( 1.0f - lerp );

                            g_WireframeVerts[numLineVerts++] = { pos0, triColour };
                            g_WireframeVerts[numLineVerts++] = { pos1, triColour };
                            g_WireframeVerts[numLineVerts++] = { pos1, triColour };
                            g_WireframeVerts[numLineVerts++] = { pos2, triColour };
                            g_WireframeVerts[numLineVerts++] = { pos2, triColour };
                            g_WireframeVerts[numLineVerts++] = { pos0, triColour };
                        }
                    }

                    ASSERT ( g_VertexBufferWireframeSize >= numLineVerts );
                    g_pd3dDeviceContext->Map(g_VertexBufferWireframe, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubRes);
                    CopyMemory(mappedSubRes.pData, &g_WireframeVerts, sizeof(g_WireframeVerts[0]) * numLineVerts);
                    g_pd3dDeviceContext->Unmap(g_VertexBufferWireframe, 0);

                    g_pd3dDeviceContext->OMSetDepthStencilState(g_DepthStencilStateOff, 0);

                    g_pd3dDeviceContext->Draw(numLineVerts, 0);

                    ASSERT ( g_VertexBufferWireframeSize >= numLineVerts );
                }
            }
        }

        // --- Render the GUI
        if (g_showDearImgui)
        {
            ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
        }

        // --- Present
        HRESULT hr;
        if (g_VsyncEnabled)
        {
            hr = g_pSwapChain->Present(1, 0);
        }
        else
        {
            hr = g_pSwapChain->Present(0, 0);
        }
        g_SwapChainOccluded = (hr == DXGI_STATUS_OCCLUDED);
    }

    // --- Cleanup
    ImGui_ImplDX11_Shutdown();
    ImGui_ImplWin32_Shutdown();
    ImGui::DestroyContext();

    CleanupDeviceD3D();
    ::DestroyWindow(hwnd);
    ::UnregisterClassW(wc.lpszClassName, wc.hInstance);

    return 0;
}

void RenderDistortionPass()
{
    // Copy the shared constant buffer.
    D3D11_MAPPED_SUBRESOURCE mappedSubRes;
    g_pd3dDeviceContext->Map(g_ConstantBuffer, 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedSubRes);
    CopyMemory(mappedSubRes.pData, &g_ConstantBufferData, sizeof(g_ConstantBufferData));
    g_pd3dDeviceContext->Unmap(g_ConstantBuffer, 0);


    PipelineState const *curPipeline = &(g_Pipeline[PipelineState::Pipeline_Deform]);

    const D3D11_VIEWPORT viewport = CD3D11_VIEWPORT(0.0f, 0.0f, (float)g_SurfaceFromObjectTextureSize, (float)g_SurfaceFromObjectTextureSize);
    g_pd3dDeviceContext->RSSetViewports(1, &viewport);

    ID3D11RenderTargetView *rtvs[4] = {
        g_TextureSurfaceFromObjectTemp0RTV,
        g_TextureSurfaceFromObjectTemp1RTV,
        g_TextureSurfaceFromObjectTemp2RTV,
        g_TextureSurfaceFromObjectTemp3RTV,
    };
    g_pd3dDeviceContext->OMSetRenderTargets(4, rtvs, nullptr);

    g_pd3dDeviceContext->VSSetShader(curPipeline->vertexShader, nullptr, 0);
    g_pd3dDeviceContext->VSSetConstantBuffers(0, 1, &g_ConstantBuffer);

    g_pd3dDeviceContext->PSSetShader(curPipeline->pixelShader, nullptr, 0);
    g_pd3dDeviceContext->PSSetConstantBuffers( 0, 1, &g_ConstantBuffer);

    g_pd3dDeviceContext->PSSetShaderResources( 0, 1, &g_TextureAlbedoSRV );
    g_pd3dDeviceContext->PSSetShaderResources( 1, 1, &g_TextureHeightSRV );
    g_pd3dDeviceContext->PSSetShaderResources( 2, 1, &g_TextureNormalSRV );
    g_pd3dDeviceContext->PSSetSamplers (0, 1, &g_SamplerLinear);

    g_pd3dDeviceContext->OMSetDepthStencilState(g_DepthStencilStateOff, 0);
    Vec4 blendFactor ( 1.0f, 1.0f, 1.0f, 1.0f );
    g_pd3dDeviceContext->OMSetBlendState(g_BlendStateOff, blendFactor.AsFloatPtr(), ~0u);

    g_pd3dDeviceContext->RSSetState(g_RasterizerState);
    g_pd3dDeviceContext->IASetInputLayout(curPipeline->inputLayout);

    ID3D11Buffer* buffers[] = { g_VertexBuffer };
    const UINT stride[] = { sizeof(VertexBufferStruct) };
    const UINT offset[] = { 0 };

    g_pd3dDeviceContext->IASetVertexBuffers(0, 1, buffers, stride, offset);

    g_pd3dDeviceContext->IASetIndexBuffer(g_IndexBuffer, DXGI_FORMAT_R32_UINT, 0); // our indices are uint32
    g_pd3dDeviceContext->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);

    g_pd3dDeviceContext->DrawIndexed((UINT)g_NumTris * 3, 0, 0);
}



// Helper functions

bool CreateDeviceD3D(HWND hWnd)
{
    // Setup swap chain
    // This is a basic setup. Optimally could use e.g. DXGI_SWAP_EFFECT_FLIP_DISCARD and handle fullscreen mode differently. See #8979 for suggestions.
    DXGI_SWAP_CHAIN_DESC sd;
    ZeroMemory(&sd, sizeof(sd));
    sd.BufferCount = 2;
    sd.BufferDesc.Width = 0;
    sd.BufferDesc.Height = 0;
    sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    sd.BufferDesc.RefreshRate.Numerator = 60;
    sd.BufferDesc.RefreshRate.Denominator = 1;
    sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
    sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
    sd.OutputWindow = hWnd;
    sd.SampleDesc.Count = 1;
    sd.SampleDesc.Quality = 0;
    sd.Windowed = TRUE;
    sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

    UINT createDeviceFlags = 0;
    //createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
    D3D_FEATURE_LEVEL featureLevel;
    const D3D_FEATURE_LEVEL featureLevelArray[2] = { D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0, };
    HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
        res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_pSwapChain, &g_pd3dDevice, &featureLevel, &g_pd3dDeviceContext);
    if (res != S_OK)
        return false;

    CreateRenderTarget();

    D3D11_RASTERIZER_DESC rasterizerDesc = CD3D11_RASTERIZER_DESC(CD3D11_DEFAULT());
    rasterizerDesc.FrontCounterClockwise = false;
    g_pd3dDevice->CreateRasterizerState(&rasterizerDesc, &g_RasterizerState);

    D3D11_SAMPLER_DESC samplerDesc = CD3D11_SAMPLER_DESC(CD3D11_DEFAULT());
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    g_pd3dDevice->CreateSamplerState ( &samplerDesc, &g_SamplerLinear );
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_POINT;
    g_pd3dDevice->CreateSamplerState ( &samplerDesc, &g_SamplerPoint );

    D3D11_DEPTH_STENCIL_DESC depthStencilDesc = CD3D11_DEPTH_STENCIL_DESC();
    depthStencilDesc.DepthEnable = true;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_GREATER_EQUAL; // we are using reverse Z!
    depthStencilDesc.StencilEnable = false;
    g_pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &g_DepthStencilState);

    depthStencilDesc.DepthEnable = false;
    depthStencilDesc.DepthWriteMask = D3D11_DEPTH_WRITE_MASK_ALL;
    depthStencilDesc.DepthFunc = D3D11_COMPARISON_ALWAYS;
    depthStencilDesc.StencilEnable = false;
    g_pd3dDevice->CreateDepthStencilState(&depthStencilDesc, &g_DepthStencilStateOff);

    D3D11_BLEND_DESC blendDesc = CD3D11_BLEND_DESC();
    blendDesc.AlphaToCoverageEnable = false;
    blendDesc.IndependentBlendEnable = false;
    blendDesc.RenderTarget[0].BlendEnable = false;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    blendDesc.RenderTarget[1] = blendDesc.RenderTarget[0];
    blendDesc.RenderTarget[2] = blendDesc.RenderTarget[0];
    blendDesc.RenderTarget[3] = blendDesc.RenderTarget[0];
    g_pd3dDevice->CreateBlendState(&blendDesc, &g_BlendStateOff);

    blendDesc.RenderTarget[0].BlendEnable = true;
    blendDesc.RenderTarget[0].SrcBlend = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlend = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOp = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].SrcBlendAlpha = D3D11_BLEND_ONE;
    blendDesc.RenderTarget[0].DestBlendAlpha = D3D11_BLEND_INV_SRC_ALPHA;
    blendDesc.RenderTarget[0].BlendOpAlpha = D3D11_BLEND_OP_ADD;
    blendDesc.RenderTarget[0].RenderTargetWriteMask = D3D11_COLOR_WRITE_ENABLE_ALL;
    g_pd3dDevice->CreateBlendState(&blendDesc, &g_BlendStateAlpha);

    return true;
}

void CleanupDeviceD3D()
{
    CleanupRenderTarget();

    SAFE_RELEASE(g_ConstantBuffer);
    SAFE_RELEASE(g_TextureSurfaceFromObjectTemp0);
    SAFE_RELEASE(g_TextureSurfaceFromObjectTemp1);
    SAFE_RELEASE(g_TextureSurfaceFromObjectTemp2);
    SAFE_RELEASE(g_TextureSurfaceFromObjectTemp3);
    SAFE_RELEASE(g_TextureSurfaceFromObject0);
    SAFE_RELEASE(g_TextureSurfaceFromObject1);
    SAFE_RELEASE(g_TextureSurfaceFromObject2);
    SAFE_RELEASE(g_TextureSurfaceFromObject3);
    SAFE_RELEASE(g_TextureTeleportMap);
    SAFE_RELEASE(g_TextureEdgefillMap);
    SAFE_RELEASE(g_TextureSurfaceFromObjectTemp0SRV);
    SAFE_RELEASE(g_TextureSurfaceFromObjectTemp1SRV);
    SAFE_RELEASE(g_TextureSurfaceFromObjectTemp2SRV);
    SAFE_RELEASE(g_TextureSurfaceFromObjectTemp3SRV);
    SAFE_RELEASE(g_TextureSurfaceFromObject0SRV);
    SAFE_RELEASE(g_TextureSurfaceFromObject1SRV);
    SAFE_RELEASE(g_TextureSurfaceFromObject2SRV);
    SAFE_RELEASE(g_TextureSurfaceFromObject3SRV);
    SAFE_RELEASE(g_TextureTeleportMapSRV);
    SAFE_RELEASE(g_TextureEdgefillMapSRV);
    SAFE_RELEASE(g_TextureSurfaceFromObjectTemp0RTV);
    SAFE_RELEASE(g_TextureSurfaceFromObjectTemp1RTV);
    SAFE_RELEASE(g_TextureSurfaceFromObjectTemp2RTV);
    SAFE_RELEASE(g_TextureSurfaceFromObjectTemp3RTV);
    SAFE_RELEASE(g_TextureSurfaceFromObject0RTV);
    SAFE_RELEASE(g_TextureSurfaceFromObject1RTV);
    SAFE_RELEASE(g_TextureSurfaceFromObject2RTV);
    SAFE_RELEASE(g_TextureSurfaceFromObject3RTV);
    SAFE_RELEASE(g_TextureHeight);
    SAFE_RELEASE(g_TextureAlbedo);
    SAFE_RELEASE(g_TextureNormal);
    SAFE_RELEASE(g_TextureHeightSRV);
    SAFE_RELEASE(g_TextureAlbedoSRV);
    SAFE_RELEASE(g_TextureNormalSRV);
    SAFE_RELEASE(g_VertexBuffer);
    SAFE_RELEASE(g_IndexBuffer);
    SAFE_RELEASE(g_VertexBufferEdgefill);
    SAFE_RELEASE(g_IndexBufferEdgefill);
    SAFE_RELEASE(g_VertexBufferWireframe);
    SAFE_RELEASE(g_RasterizerState);
    SAFE_RELEASE(g_SamplerLinear);
    SAFE_RELEASE(g_SamplerPoint);
    SAFE_RELEASE(g_DepthStencilState);
    SAFE_RELEASE(g_DepthStencilStateOff);
    SAFE_RELEASE(g_BlendStateAlpha);
    SAFE_RELEASE(g_BlendStateOff);
    SAFE_RELEASE(g_pSwapChain);
    SAFE_RELEASE(g_pd3dDeviceContext);
    SAFE_RELEASE(g_pd3dDevice);

    for ( int i = 0; i < PipelineState::Pipeline_COUNT; i++ )
    {
        SAFE_RELEASE(g_Pipeline[i].vertexShader);
        SAFE_RELEASE(g_Pipeline[i].pixelShader);
        SAFE_RELEASE(g_Pipeline[i].inputLayout);
    }
}

void CreateRenderTarget()
{
    ID3D11Texture2D* pBackBuffer;
    g_pSwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
    g_pd3dDevice->CreateRenderTargetView(pBackBuffer, nullptr, &g_mainRenderTargetView);
    pBackBuffer->Release();


    D3D11_TEXTURE2D_DESC backBufferDesc;
    pBackBuffer->GetDesc(&backBufferDesc);
    g_CurrentWidth = backBufferDesc.Width;
    g_CurrentHeight = backBufferDesc.Height;

    // Dear ImGui doesn't need a depth/stencil buffer, but Gargaj does.
    // The old 24-bit-depth-8-bit-stencil is archaic.
    // A lot of graphics cards emulate the stencil buffer anyway.
    // Just use float32 depth buffer and no stencil.
    D3D11_TEXTURE2D_DESC depthDesc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_D32_FLOAT, g_CurrentWidth, g_CurrentHeight, 1, 1, D3D11_BIND_DEPTH_STENCIL);
    if (g_pd3dDevice->CreateTexture2D(&depthDesc, nullptr, &g_DepthStencil) != S_OK)
    {
        return;
    }

    D3D11_DEPTH_STENCIL_VIEW_DESC descDSV = CD3D11_DEPTH_STENCIL_VIEW_DESC(D3D11_DSV_DIMENSION_TEXTURE2D, DXGI_FORMAT_D32_FLOAT);
    if (g_pd3dDevice->CreateDepthStencilView(g_DepthStencil, &descDSV, &g_DepthStencilView) != S_OK)
    {
        return;
    }
}

void CleanupRenderTarget()
{
    SAFE_RELEASE(g_mainRenderTargetView);
    SAFE_RELEASE(g_DepthStencil);
    SAFE_RELEASE(g_DepthStencilView);
}

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

// Win32 message handler
// You can read the io.WantCaptureMouse, io.WantCaptureKeyboard flags to tell if dear imgui wants to use your inputs.
// - When io.WantCaptureMouse is true, do not dispatch mouse input data to your main application, or clear/overwrite your copy of the mouse data.
// - When io.WantCaptureKeyboard is true, do not dispatch keyboard input data to your main application, or clear/overwrite your copy of the keyboard data.
// Generally you may always pass all inputs to dear imgui, and hide them from your application based on those two flags.
LRESULT WINAPI WndProc(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam)
{
    if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam))
        return true;

    switch (msg)
    {
    case WM_KEYDOWN:
    {
        switch (wParam)
        {
        case VK_ESCAPE:
            if (g_showIntroWindow)
            {
                g_showIntroWindow = false;
            }
            else
            {
                g_showDearImgui = !g_showDearImgui;
            }
            break;
        case VK_SPACE:
            g_boneAnimPaused = !g_boneAnimPaused;
            break;
        }
    } break;

    case WM_SIZE:
        if (wParam == SIZE_MINIMIZED)
            return 0;
        g_ResizeWidth = (UINT)LOWORD(lParam); // Queue resize
        g_ResizeHeight = (UINT)HIWORD(lParam);
        return 0;

    case WM_SYSCOMMAND:
    {
        if ((wParam & 0xfff0) == SC_KEYMENU) // Disable ALT application menu
            return 0;

        switch ( wParam )
        {
        case SC_SCREENSAVE:
        case SC_MONITORPOWER:
            // If this is a non-interactive demo,
            // make sure power saving doesn't turn the screen
            // off just because nobody touched a key for a while.
            // If this is an interactive game you probably don't want this.
            return 0;
        }
    } break;

    case WM_DESTROY:
        ::PostQuitMessage(0);
        return 0;
    }
    return ::DefWindowProcW(hWnd, msg, wParam, lParam);
}


void CreateModel()
{
    if ( g_FreeArrays )
    {
        delete[] g_VertexData;
        delete[] g_VertexDataTemp;
        delete[] g_IndexData;
    }

    SAFE_RELEASE(g_VertexBuffer);
    SAFE_RELEASE(g_IndexBuffer);
    SAFE_RELEASE(g_VertexBufferEdgefill);
    SAFE_RELEASE(g_IndexBufferEdgefill);
    SAFE_RELEASE(g_VertexBufferWireframe);

    g_FreeArrays = false;
    g_MeshNumber_Current = g_MeshNumber;
    g_NumSegmentsAround_Current = g_NumSegmentsAround;
    g_NumSegmentsLong_Current = g_NumSegmentsLong;
    g_MiddleTubeLength_Current = g_MiddleTubeLength;
    g_MeshRadius_Current = g_MeshRadius;
    g_SurfaceThickness_Current = g_SurfaceThickness;

    if ( ( g_MeshNumber == MeshType_Tube ) ||
         ( g_MeshNumber == MeshType_TubePinched ) )
    {
        bool pinchedTube = ( g_MeshNumber == MeshType_TubePinched );

        // Tube with rounded ends.
        // The ends are triangles that get stitched together.
        //
        // MeshType_TubePinched has fully triangular ends that
        // meet at a point. However, this tends to produce tornados,
        // which this type is specifically here to demonstrate.
        // 
        //         <--numSegmentsLong-->
        //        _+---+---+---+---+---+_
        //      _- |   |   |   |   |   | -_
        //    _+_  |   |   |   |   |   |  _+_
        //  _- | -_|   |   |   |   |   |_- | -_
        // +---+---+---+---+---+---+---+---+---+
        //      _-^|   |   |   |   |   |^-_
        //    _+_  |   |   |   |   |   |  _+_
        //  _- | -_|   |   |   |   |   |_- | -_
        // +---+---+---+---+---+---+---+---+---+
        // ...etc
        //    numSegmentsAroundQuarter:<------->
        //
        // The whole tube goes from 0.1 to 0.7 in UV coordinates to give room for teleports at the edges and ends.
        //
        // MeshType_Tube has ends that do not include the final triangle.
        // Instead they have two end caps that avoid excessive distortion.
        // 
        //         <--numSegmentsLong-->
        //        _+---+---+---+---+---+_      +---+
        //      _- |   |   |   |   |   | -_   / \ / \
        //     +_  |   |   |   |   |   |  _+ +---+---+
        //     | -_|   |   |   |   |   |_- |  \_/_\_/
        //     +---+---+---+---+---+---+---+   _____
        //      _-^|   |   |   |   |   |^-_   / \ / \
        //     +_  |   |   |   |   |   |  _+ +---+---+
        //     | -_|   |   |   |   |   |_- |  \ / \ /
        //     +---+---+---+---+---+---+---+   +---+
        // ...etc
        //    numSegmentsAroundQuarter:<------->
        //
        // The whole tube goes from 0.1 to 0.7 in UV coordinates to give room for teleports at the edges and ends,
        // and some space for the end caps.

        int numSegmentsAroundQuarter = Max ( 1, (g_NumSegmentsAround+2) / 4 ); // How many at each end cap.

        g_NumVerts = ( (g_NumSegmentsLong+1) * (g_NumSegmentsAround+1) ); // body
        g_NumTris = 2 * g_NumSegmentsLong * g_NumSegmentsAround; // body
        if ( pinchedTube )
        {
            g_NumVerts += ( ( numSegmentsAroundQuarter - 1 ) * 2 + 1 ) * 2 * ( g_NumSegmentsAround ); // ends
            g_NumTris += 2 * ( ( numSegmentsAroundQuarter - 1 ) * 2 + 1 ) * g_NumSegmentsAround; // ends
        }
        else
        {
            g_NumVerts += ( ( numSegmentsAroundQuarter - 1 ) * 2 ) * 2 * ( g_NumSegmentsAround ); // ends
            g_NumTris += 2 * ( ( numSegmentsAroundQuarter - 1 ) * 2 ) * g_NumSegmentsAround; // ends
            g_NumVerts += 2 * ( 1 + g_NumSegmentsAround ); // caps
            g_NumTris += 2 * ( g_NumSegmentsAround ); // caps
        }

        g_VertexData = new VertexBufferStruct [g_NumVerts];
        g_VertexDataTemp = new VertexBufferStruct [g_NumVerts];
        g_IndexData = new UINT[g_NumTris * 3];
        g_FreeArrays = true;

        // Body.

        VertexBufferStruct* curVert = g_VertexData;
        UINT* curIndex = g_IndexData;

        float endCapLengthInMeters = PI * 0.5f * g_MeshRadius;

        {
            // Total V length (in meters) = g_MiddleTubeLength + 2 * endCapLengthInMeters
            // Middle section starts at endCapLengthInMeters
            float vLength = g_MiddleTubeLength + 2.0f * endCapLengthInMeters;

            // Additional scale by 0.6 and offset by 0.1 allows room at the edges for edgefill/teleport.
            float vScale = 0.6f * g_MiddleTubeLength / ( vLength * (float)g_NumSegmentsLong );
            float vOffset = 0.1f + 0.6f * ( endCapLengthInMeters / vLength );

            for ( int length = 0; length < g_NumSegmentsLong + 1; length++ )
            {
                for ( int around = 0; around < g_NumSegmentsAround + 1; around++ )
                {
                    float length01 = ( (float)length / (float)g_NumSegmentsLong );
                    float angle01 = ( (float)around / (float)g_NumSegmentsAround );
                    float angle = 2.0f * PI * angle01;

                    // Put the seam on the bottom, going anticlockwise.
                    curVert->Pos.x =  sinf ( angle ) * g_MeshRadius;
                    curVert->Pos.y = -cosf ( angle ) * g_MeshRadius;
                    curVert->Pos.z = g_MiddleTubeLength * length01;

                    curVert->TexCoord.x = 0.1f + 0.8f * angle01; // leave room for teleports.
                    curVert->TexCoord.y = vOffset + vScale * (float)length;
                    curVert->TexCoord.z = 1.0f; // full extrusion.

                    curVert->Normal.x =  sinf ( angle ) * g_SurfaceThickness;
                    curVert->Normal.y = -cosf ( angle ) * g_SurfaceThickness;
                    curVert->Normal.z = 0.0f;

                    // Bones weights are somewhat evenly spaced down the tube.
                    curVert->BoneWeights.x = 0.0f;
                    curVert->BoneWeights.y = 0.0f;
                    curVert->BoneWeights.z = 0.0f;
                    curVert->BoneWeights.w = 0.0f;
                    float boneDistance01 = length01;
                    if ( boneDistance01 < 0.1f )
                    {
                        // 0.0 - 0.1
                        curVert->BoneWeights.x = 1.0f;
                    }
                    else if ( boneDistance01 < 0.2f )
                    {
                        // 0.1 - 0.2
                        float frac = ( 0.2f - boneDistance01 ) / 0.1f;
                        curVert->BoneWeights.x = frac;
                        curVert->BoneWeights.y = 1.0f - frac;
                    }
                    else if ( boneDistance01 < 0.4f )
                    {
                        // 0.2 - 0.4
                        curVert->BoneWeights.y = 1.0f;
                    }
                    else if ( boneDistance01 < 0.6f )
                    {
                        // 0.4 - 0.6
                        float frac = ( 0.6f - boneDistance01 ) / 0.2f;
                        curVert->BoneWeights.y = frac;
                        curVert->BoneWeights.z = 1.0f - frac;
                    }
                    else if ( boneDistance01 < 0.8f )
                    {
                        // 0.6 - 0.8
                        curVert->BoneWeights.z = 1.0f;
                    }
                    else if ( boneDistance01 < 0.9f )
                    {
                        // 0.8 - 0.9
                        float frac = ( 0.9f - boneDistance01 ) / 0.1f;
                        curVert->BoneWeights.z = frac;
                        curVert->BoneWeights.w = 1.0f - frac;
                    }
                    else
                    {
                        curVert->BoneWeights.w = 1.0f;
                    }

                    curVert++;
                }
            }
        }

        // End triangle verts
        for ( int capNum = 0; capNum < 2; capNum++ )
        {
            // Total V length (in meters) = g_MiddleTubeLength + 2 * ( PI/2 * g_MeshRadius )
            // Middle section starts at PI/2 * g_MeshRadius
            float vLength = g_MiddleTubeLength + 2.0f * endCapLengthInMeters;

            // Additional scale by 0.8 and offset by 0.1 allows room at the edges for edgefill/teleport.
            float vScale = 0.6f * endCapLengthInMeters / ( vLength * (float)numSegmentsAroundQuarter );
            float vOffset = 0.1f + 0.6f * ( endCapLengthInMeters / vLength );

            if ( capNum == 1 )
            {
                vScale = -vScale;
                vOffset = 0.1f + 0.6f * ( ( endCapLengthInMeters + g_MiddleTubeLength ) / vLength );
            }

            for ( int length = 1; length <= numSegmentsAroundQuarter; length++ )
            {
                for ( int around = 0; around < g_NumSegmentsAround; around++ )
                {
                    float angle01 = ( (float)around / (float)g_NumSegmentsAround );
                    float angle = 2.0f * PI * angle01;
                    float endAngle01 = (float)length / (float)numSegmentsAroundQuarter;
                    float endAngle = 0.5f * PI * endAngle01;

                    float centerPos = 0.0f;
                    if ( capNum == 1 )
                    {
                        centerPos = g_MiddleTubeLength;
                        endAngle = -endAngle;
                    }

                    if ( pinchedTube || ( length < numSegmentsAroundQuarter ) )
                    {
                        // The "lower" vertex of the strip.
                        curVert->Pos.x =  sinf ( angle ) * cosf ( endAngle ) * g_MeshRadius;
                        curVert->Pos.y = -cosf ( angle ) * cosf ( endAngle ) * g_MeshRadius;
                        curVert->Pos.z = centerPos - sinf ( endAngle ) * g_MeshRadius;

                        curVert->TexCoord.x = 0.1f + 0.8f * angle01; // leave room for teleports.
                        curVert->TexCoord.y = vOffset - vScale * (float)length;
                        curVert->TexCoord.z = 1.0f; // full extrusion.

                        curVert->Normal.x =  sinf ( angle ) * cosf ( endAngle ) * g_SurfaceThickness;
                        curVert->Normal.y = -cosf ( angle ) * cosf ( endAngle ) * g_SurfaceThickness;
                        curVert->Normal.z = -sinf ( endAngle ) * g_SurfaceThickness;

                        if ( capNum == 0 )
                        {
                            curVert->BoneWeights.x = 1.0f;
                            curVert->BoneWeights.y = 0.0f;
                            curVert->BoneWeights.z = 0.0f;
                            curVert->BoneWeights.w = 0.0f;
                        }
                        else
                        {
                            curVert->BoneWeights.x = 0.0f;
                            curVert->BoneWeights.y = 0.0f;
                            curVert->BoneWeights.z = 0.0f;
                            curVert->BoneWeights.w = 1.0f;
                        }

                        curVert++;
                    }

                    if ( length < numSegmentsAroundQuarter )
                    {
                        // The "upper" vertex of the strip.
                        float texAngle01 = angle01;

                        angle01 += ( 1.0f / (float)g_NumSegmentsAround );
                        angle = 2.0f * PI * angle01;

                        curVert->Pos.x =  sinf ( angle ) * cosf ( endAngle ) * g_MeshRadius;
                        curVert->Pos.y = -cosf ( angle ) * cosf ( endAngle ) * g_MeshRadius;
                        curVert->Pos.z = centerPos - sinf ( endAngle ) * g_MeshRadius;

                        // Tex coord is more subtle - the U position only advances by some of the way.
                        texAngle01 += ( 1.0f / (float)g_NumSegmentsAround ) * cosf ( endAngle );
                        curVert->TexCoord.x = 0.1f + 0.8f * texAngle01;
                        curVert->TexCoord.y = vOffset - vScale * (float)length;
                        curVert->TexCoord.z = 1.0f; // full extrusion.

                        curVert->Normal.x =  sinf ( angle ) * cosf ( endAngle ) * g_SurfaceThickness;
                        curVert->Normal.y = -cosf ( angle ) * cosf ( endAngle ) * g_SurfaceThickness;
                        curVert->Normal.z = -sinf ( endAngle ) * g_SurfaceThickness;

                        if ( capNum == 0 )
                        {
                            curVert->BoneWeights.x = 1.0f;
                            curVert->BoneWeights.y = 0.0f;
                            curVert->BoneWeights.z = 0.0f;
                            curVert->BoneWeights.w = 0.0f;
                        }
                        else
                        {
                            curVert->BoneWeights.x = 0.0f;
                            curVert->BoneWeights.y = 0.0f;
                            curVert->BoneWeights.z = 0.0f;
                            curVert->BoneWeights.w = 1.0f;
                        }

                        curVert++;
                    }
                }
            }
        }

        // End cap verts.
        if ( !pinchedTube )
        {
            for ( int capNum = 0; capNum < 2; capNum++ )
            {
                int length = numSegmentsAroundQuarter - 1;
                for ( int around = 0; around < g_NumSegmentsAround; around++ )
                {
                    float angle01 = ( (float)around / (float)g_NumSegmentsAround );
                    float angle = 2.0f * PI * angle01;
                    float endAngle01 = (float)length / (float)numSegmentsAroundQuarter;
                    float endAngle = 0.5f * PI * endAngle01;

                    float centerPos = 0.0f;
                    if ( capNum == 1 )
                    {
                        centerPos = g_MiddleTubeLength;
                        endAngle = -endAngle;
                    }

                    curVert->Pos.x =  sinf ( angle ) * cosf ( endAngle ) * g_MeshRadius;
                    curVert->Pos.y = -cosf ( angle ) * cosf ( endAngle ) * g_MeshRadius;
                    curVert->Pos.z = centerPos - sinf ( endAngle ) * g_MeshRadius;

                    curVert->TexCoord.x = 0.25f + 0.1f * sinf ( angle );
                    curVert->TexCoord.y = 0.85f + 0.1f * cosf ( angle );
                    curVert->TexCoord.z = 1.0f; // full extrusion.
                    if ( capNum == 1 )
                    {
                        // Translated in the map and flipped.
                        curVert->TexCoord.x = 0.75f - 0.1f * sinf ( angle );
                    }

                    curVert->Normal.x =  sinf ( angle ) * cosf ( endAngle ) * g_SurfaceThickness;
                    curVert->Normal.y = -cosf ( angle ) * cosf ( endAngle ) * g_SurfaceThickness;
                    curVert->Normal.z = -sinf ( endAngle ) * g_SurfaceThickness;

                    if ( capNum == 0 )
                    {
                        curVert->BoneWeights.x = 1.0f;
                        curVert->BoneWeights.y = 0.0f;
                        curVert->BoneWeights.z = 0.0f;
                        curVert->BoneWeights.w = 0.0f;
                    }
                    else
                    {
                        curVert->BoneWeights.x = 0.0f;
                        curVert->BoneWeights.y = 0.0f;
                        curVert->BoneWeights.z = 0.0f;
                        curVert->BoneWeights.w = 1.0f;
                    }

                    curVert++;
                }

                // End point.
                {
                    curVert->Pos.x = 0.0f;
                    curVert->Pos.y = 0.0f;
                    curVert->Pos.z = -g_MeshRadius;
                    if ( capNum == 1 )
                    {
                        curVert->Pos.z = g_MiddleTubeLength + g_MeshRadius;
                    }

                    curVert->TexCoord.x = 0.25f;
                    curVert->TexCoord.y = 0.85f;
                    curVert->TexCoord.z = 1.0f; // full extrusion.
                    if ( capNum == 1 )
                    {
                        // Higher in the map
                        curVert->TexCoord.x = 0.75f;
                    }

                    curVert->Normal.x = 0.0f;
                    curVert->Normal.y = 0.0f;
                    curVert->Normal.z = -g_SurfaceThickness;
                    if ( capNum == 1 )
                    {
                        curVert->Normal.z = g_SurfaceThickness;
                    }

                    if ( capNum == 0 )
                    {
                        curVert->BoneWeights.x = 1.0f;
                        curVert->BoneWeights.y = 0.0f;
                        curVert->BoneWeights.z = 0.0f;
                        curVert->BoneWeights.w = 0.0f;
                    }
                    else
                    {
                        curVert->BoneWeights.x = 0.0f;
                        curVert->BoneWeights.y = 0.0f;
                        curVert->BoneWeights.z = 0.0f;
                        curVert->BoneWeights.w = 1.0f;
                    }

                    curVert++;
                }

            }
        }

        // Now the triangles for the tube
        for ( int length = 0; length < g_NumSegmentsLong; length++ )
        {
            for ( int around = 0; around < g_NumSegmentsAround; around++ )
            {
                *curIndex++ = ( length      * (g_NumSegmentsAround + 1)) + around;
                *curIndex++ = ( length      * (g_NumSegmentsAround + 1)) + around + 1;
                *curIndex++ = ((length + 1) * (g_NumSegmentsAround + 1)) + around;
                *curIndex++ = ((length + 1) * (g_NumSegmentsAround + 1)) + around;
                *curIndex++ = ( length      * (g_NumSegmentsAround + 1)) + around + 1;
                *curIndex++ = ((length + 1) * (g_NumSegmentsAround + 1)) + around + 1;
            }
        }

        // First end tris
        int vertCapStart = ( g_NumSegmentsLong + 1 ) * ( g_NumSegmentsAround + 1);
        for ( int around = 0; around < g_NumSegmentsAround; around++ )
        {
            int v00 = around;
            int v01 = v00 + 1;
            for ( int length = 0; length < numSegmentsAroundQuarter - 1; length++ )
            {
                int v10 = vertCapStart + ( ( length * g_NumSegmentsAround ) + around ) * 2;
                int v11 = v10 + 1;

                *curIndex++ = v00;
                *curIndex++ = v10;
                *curIndex++ = v01;
                *curIndex++ = v01;
                *curIndex++ = v10;
                *curIndex++ = v11;

                v00 = v10;
                v01 = v11;
            }
            if ( pinchedTube )
            {
                // Last tri.
                *curIndex++ = v00;
                *curIndex++ = vertCapStart + ( ( ( numSegmentsAroundQuarter - 1 ) * g_NumSegmentsAround ) * 2 + around );
                *curIndex++ = v01;
            }
        }

        // Second end tris
        if ( pinchedTube )
        {
            vertCapStart += ( ( numSegmentsAroundQuarter - 1 ) * 2 + 1 ) * g_NumSegmentsAround;
        }
        else
        {
            vertCapStart += ( ( numSegmentsAroundQuarter - 1 ) * 2 ) * g_NumSegmentsAround;
        }
        for ( int around = 0; around < g_NumSegmentsAround; around++ )
        {
            int v00 = around + ( g_NumSegmentsAround + 1 ) * g_NumSegmentsLong;
            int v01 = v00 + 1;
            for ( int length = 0; length < numSegmentsAroundQuarter - 1; length++ )
            {
                int v10 = vertCapStart + ( ( length * g_NumSegmentsAround ) + around ) * 2;
                int v11 = v10 + 1;

                // Flipped winding.
                *curIndex++ = v00;
                *curIndex++ = v01;
                *curIndex++ = v10;
                *curIndex++ = v10;
                *curIndex++ = v01;
                *curIndex++ = v11;

                v00 = v10;
                v01 = v11;
            }
            if ( pinchedTube )
            {
                // Last tri.
                *curIndex++ = v01;
                *curIndex++ = vertCapStart + ( ( ( numSegmentsAroundQuarter - 1 ) * g_NumSegmentsAround ) * 2 + around );
                *curIndex++ = v00;
            }
        }

        if ( !pinchedTube )
        {
            // End caps.
            vertCapStart = ( (g_NumSegmentsLong+1) * (g_NumSegmentsAround+1) ) + ( ( numSegmentsAroundQuarter - 1 ) * 2 ) * 2 * ( g_NumSegmentsAround );
            int v00 = vertCapStart;
            int v01 = vertCapStart + g_NumSegmentsAround;
            int v02 = vertCapStart + g_NumSegmentsAround - 1;
            int v10 = v00 + g_NumSegmentsAround + 1;
            int v11 = v01 + g_NumSegmentsAround + 1;
            int v12 = v02 + g_NumSegmentsAround + 1;
            for ( int width = 0; width < g_NumSegmentsAround; width++ )
            {
                *curIndex++ = v00;
                *curIndex++ = v02;
                *curIndex++ = v01;
                *curIndex++ = v10;
                *curIndex++ = v11;
                *curIndex++ = v12;
                v02 = v00;
                v00++;
                v12 = v10;
                v10++;
            }
        }

        ASSERT ( curVert == g_VertexData + g_NumVerts );
        ASSERT ( curIndex == g_IndexData + ( g_NumTris * 3 ) );
    }
    else if ( ( g_MeshNumber == MeshType_Cube ) ||
              ( g_MeshNumber == MeshType_CubePinched ) )
    {
        // An inflated cube with seams.

        bool pinchedCube = ( g_MeshNumber == MeshType_CubePinched );


        float numSegmentsPerEdge = g_NumSegmentsAround / 4;

        // Each cube side is numSegmentsPerEdge in quads.
        //
        // Layout is this, with this ordering of verts/faces:
        //
        //     +---+                +---+           - 0.1
        //     | 4 |                |+y |
        //     +---+                +---+           - 0.3
        // +---+---+---+---+    +---+---+---+---+   - 0.4
        // | 0 | 1 | 2 | 3 |    |-z |+x |+z |-x |          Texture V
        // +---+---+---+---+    +---+---+---+---+   - 0.6
        //     +---+                +---+           - 0.7
        //     | 5 |                |-y |
        //     +---+                +---+           - 0.9
        // 
        //                      |   |   |   |   |
        //          Texture U: 0.1 0.3 0.5 0.7 0.9
        // 
        // The cube is then "inflated" into close to a sphere to make the seams smooth.
        // UV map does not go all the way to the edge to allow space for teleport * edgefill.
        //
        // The "pinched" version fuses the top and bottom squares to the
        // middle, which absolutely looks like it should work.
        // The problem is that the corner vertices do not have well-defined tangent spaces
        // and the huge distortion causes horrible "poles".
        // 
        //
        //     +---+                +---+           - 0.2
        //     | 4 |                |+y |
        // +---+---+---+---+    +---+---+---+---+   - 0.4
        // | 0 | 1 | 2 | 3 |    |-z |+x |+z |-x |          Texture V
        // +---+---+---+---+    +---+---+---+---+   - 0.6
        //     | 5 |                |-y |
        //     +---+                +---+           - 0.8
        // 
        //                      |   |   |   |   |
        //          Texture U: 0.1 0.3 0.5 0.7 0.9
        // 
        // This is included here for demonstration of this problem.

        // 0,1,2,3
        g_NumVerts = (numSegmentsPerEdge + 1) * (4 * numSegmentsPerEdge + 1);
        // 4, 5
        g_NumVerts += 2 * (numSegmentsPerEdge + 1) * (numSegmentsPerEdge + 1);

        g_NumTris = numSegmentsPerEdge * numSegmentsPerEdge * 6 * 2;

        g_FreeArrays = true;
        g_VertexData = new VertexBufferStruct[g_NumVerts];
        g_VertexDataTemp = new VertexBufferStruct[g_NumVerts];
        g_IndexData = new UINT[g_NumTris * 3];

        VertexBufferStruct* curVert = g_VertexData;
        for ( int face = 0; face < 4; face++ )
        {
            for ( int width = 0; width < numSegmentsPerEdge; width++ )
            {
                for ( int height = 0; height < numSegmentsPerEdge + 1; height++ )
                {
                    VertexBufferStruct vert;
                    float hfrac = (float)width / (float)numSegmentsPerEdge;
                    float vfrac = (float)height / (float)numSegmentsPerEdge;
                    float hpos = -1.0f + 2.0f * hfrac;
                    float ypos = -1.0f + 2.0f * vfrac;

                    // Here, we generate data for the uninflated cube.
                    switch ( face )
                    {
                    case 0: // -z
                        vert.Pos        = Vec3 (  hpos,  ypos, -1.0f );
                        vert.Normal     = Vec3 (  0.0f,  0.0f, -1.0f );
                        break;
                    case 1: // +x
                        vert.Pos        = Vec3 (  1.0f,  ypos,  hpos );
                        vert.Normal     = Vec3 (  1.0f,  0.0f,  0.0f );
                        break;
                    case 2: // +z
                        vert.Pos        = Vec3 ( -hpos,  ypos,  1.0f );
                        vert.Normal     = Vec3 (  0.0f,  0.0f,  1.0f );
                        break;
                    case 3: // -x
                        vert.Pos        = Vec3 ( -1.0f,  ypos, -hpos );
                        vert.Normal     = Vec3 ( -1.0f,  0.0f,  0.0f );
                        break;
                    }
                    vert.TexCoord = Vec3 (
                        ( hfrac + (float)face ) * 0.2f + 0.1f,
                        vfrac * -0.2f + 0.6f,
                        1.0f );

                    // TODO: bones.
                    vert.BoneWeights = Vec4 ( 1.0f, 0.0f, 0.0f, 0.0f );

                    *curVert++ = vert;
                }
            }
        }
        // Final edge of face 3.
        for ( int height = 0; height < numSegmentsPerEdge + 1; height++ )
        {
            VertexBufferStruct vert;
            float vfrac = (float)height / (float)numSegmentsPerEdge;
            float ypos = -1.0f + 2.0f * vfrac;
            vert.Pos        = Vec3 ( -1.0f,  ypos, -1.0f );
            vert.Normal     = Vec3 ( -1.0f,  0.0f,  0.0f );

            vert.TexCoord = Vec3 (
                0.9f,
                vfrac * -0.2f + 0.6f,
                1.0f );

            // TODO: bones.
            vert.BoneWeights = Vec4 ( 1.0f, 0.0f, 0.0f, 0.0f );

            *curVert++ = vert;
        }

        for ( int face = 4; face <= 5; face++ )
        {
            float face4VTop = 0.3f;
            float face5VTop = 0.9f;
            if ( pinchedCube )
            {
                face4VTop = 0.4f;
                face5VTop = 0.8f;
            }

            for ( int width = 0; width < numSegmentsPerEdge + 1; width++ )
            {
                for ( int height = 0; height < numSegmentsPerEdge + 1; height++ )
                {
                    VertexBufferStruct vert;
                    float hfrac = (float)width / (float)numSegmentsPerEdge;
                    float vfrac = (float)height / (float)numSegmentsPerEdge;
                    float hpos = -1.0f + 2.0f * hfrac;
                    float vpos = -1.0f + 2.0f * vfrac;

                    // Here, we generate data for the uninflated cube.
                    switch ( face )
                    {
                    case 4: // +y
                        vert.Pos        = Vec3 ( -vpos,  1.0f,  hpos );
                        vert.Normal     = Vec3 (  0.0f,  1.0f,  0.0f );
                        vert.TexCoord   = Vec3 ( 0.3f + 0.2f * hfrac,
                                                 face4VTop - 0.2f * vfrac,
                                                 1.0f );
                        break;
                    case 5: // -y
                        vert.Pos        = Vec3 (  vpos, -1.0f,  hpos );
                        vert.Normal     = Vec3 (  0.0f, -1.0f,  0.0f );
                        vert.TexCoord   = Vec3 ( 0.3f + 0.2f * hfrac,
                                                 face5VTop - 0.2f * vfrac,
                                                 1.0f );
                        break;
                    }

                    // TODO: bones.
                    vert.BoneWeights = Vec4 ( 1.0f, 0.0f, 0.0f, 0.0f );

                    *curVert++ = vert;
                }
            }
        }

        ASSERT ( curVert == g_VertexData + g_NumVerts );

        UINT *curInd = g_IndexData;
        // Faces 0, 1, 2, 3
        for ( int face = 0; face < 4; face++ )
        {
            for ( int width = 0; width < numSegmentsPerEdge; width++ )
            {
                for ( int height = 0; height < numSegmentsPerEdge; height++ )
                {
                    int v00 = ( face * numSegmentsPerEdge + width ) * ( numSegmentsPerEdge + 1 ) + height;
                    int v01 = v00 + 1;
                    int v10 = v00 + ( numSegmentsPerEdge + 1 );
                    int v11 = v10 + 1;

                    *curInd++ = v00;
                    *curInd++ = v01;
                    *curInd++ = v10;
                    *curInd++ = v10;
                    *curInd++ = v01;
                    *curInd++ = v11;
                }
            }
        }

        // Face 4
        int indexOffset = ( ( 1 + 4 * numSegmentsPerEdge ) * ( numSegmentsPerEdge + 1 ) );
        for ( int width = 0; width < numSegmentsPerEdge; width++ )
        {
            for ( int height = 0; height < numSegmentsPerEdge; height++ )
            {
                int v00 = indexOffset + height + ( width * ( numSegmentsPerEdge + 1 ) );
                int v01 = v00 + 1;
                int v10 = v00 + ( numSegmentsPerEdge + 1 );
                int v11 = v10 + 1;

                if ( pinchedCube && ( height == 0 ) )
                {
                    // The bottom edge of face 4 shares the same verts as the top edge of face 1
                    v00 = ( 1 * numSegmentsPerEdge + width ) * ( numSegmentsPerEdge + 1 ) + numSegmentsPerEdge;
                    v10 = v00 + ( numSegmentsPerEdge + 1 );
                }

                *curInd++ = v00;
                *curInd++ = v01;
                *curInd++ = v10;
                *curInd++ = v10;
                *curInd++ = v01;
                *curInd++ = v11;
            }
        }

        // Face 5
        indexOffset += ( numSegmentsPerEdge + 1 ) * ( numSegmentsPerEdge + 1 );
        for ( int width = 0; width < numSegmentsPerEdge; width++ )
        {
            for ( int height = 0; height < numSegmentsPerEdge; height++ )
            {
                int v00 = indexOffset + height + ( width * ( numSegmentsPerEdge + 1 ) );
                int v01 = v00 + 1;
                int v10 = v00 + ( numSegmentsPerEdge + 1 );
                int v11 = v10 + 1;

                if ( pinchedCube && ( height == numSegmentsPerEdge - 1 ) )
                {
                    // The top edge of face 5 shares the same verts as the bottom edge of face 1
                    v01 = ( 1 * numSegmentsPerEdge + width ) * ( numSegmentsPerEdge + 1 );
                    v11 = v01 + ( numSegmentsPerEdge + 1 );
                }

                *curInd++ = v00;
                *curInd++ = v01;
                *curInd++ = v10;
                *curInd++ = v10;
                *curInd++ = v01;
                *curInd++ = v11;
            }
        }

        ASSERT ( curInd == g_IndexData + 3 * g_NumTris );

        // Now inflate from cube to sphere.
        for ( int i = 0; i < g_NumVerts; i++ )
        {
            curVert = &(g_VertexData[i]);

            Vec3 unitNormal = curVert->Pos.GetNormalise();
            curVert->Pos = unitNormal * g_MeshRadius;
            curVert->Normal = unitNormal * g_SurfaceThickness;
        }
    }

    GenerateTangentSpace();

    g_NumSegmentsAround_Current = g_NumSegmentsAround;
    g_NumSegmentsLong_Current = g_NumSegmentsLong;

    D3D11_BUFFER_DESC bufferDesc = CD3D11_BUFFER_DESC((UINT)sizeof(VertexBufferStruct) * g_NumVerts, D3D11_BIND_VERTEX_BUFFER);
    D3D11_SUBRESOURCE_DATA vbSubData = { 0 };

    vbSubData.pSysMem = (void*)g_VertexData;
    if (g_pd3dDevice->CreateBuffer(&bufferDesc, &vbSubData, &g_VertexBuffer) != S_OK)
    {
        return;
    }

    D3D11_BUFFER_DESC ibBufferDesc = CD3D11_BUFFER_DESC((UINT)sizeof(g_IndexData[0]) * g_NumTris * 3, D3D11_BIND_INDEX_BUFFER);
    D3D11_SUBRESOURCE_DATA ibSubData = { 0 };
    ibSubData.pSysMem = (char*)g_IndexData;
    if (g_pd3dDevice->CreateBuffer(&ibBufferDesc, &ibSubData, &g_IndexBuffer) != S_OK)
    {
        return;
    }

    // Edgefill is just a single quad.

    VertexBufferStruct edgefillVB[4];
    edgefillVB[0].Pos = Vec3 ( 0.0f, 0.0f, 0.0f );
    edgefillVB[0].TexCoord = Vec3 ( 0.0f, 0.0f, 0.0f );
    edgefillVB[1].Pos = Vec3 ( 1.0f, 0.0f, 0.0f );
    edgefillVB[1].TexCoord = Vec3 ( 1.0f, 0.0f, 0.0f );
    edgefillVB[2].Pos = Vec3 ( 0.0f, 1.0f, 0.0f );
    edgefillVB[2].TexCoord = Vec3 ( 0.0f, 1.0f, 0.0f );
    edgefillVB[3].Pos = Vec3 ( 1.0f, 1.0f, 0.0f );
    edgefillVB[3].TexCoord = Vec3 ( 1.0f, 1.0f, 0.0f );

    bufferDesc = CD3D11_BUFFER_DESC((UINT)sizeof(VertexBufferStruct) * 4, D3D11_BIND_VERTEX_BUFFER);
    vbSubData = { 0 };
    vbSubData.pSysMem = (void*)edgefillVB;
    if (g_pd3dDevice->CreateBuffer(&bufferDesc, &vbSubData, &g_VertexBufferEdgefill) != S_OK)
    {
        return;
    }

    UINT edgefillIB[6] = { 0, 1, 2, 2, 1, 3 };

    ibBufferDesc = CD3D11_BUFFER_DESC((UINT)sizeof(edgefillIB[0]) * 6, D3D11_BIND_INDEX_BUFFER);
    ibSubData = { 0 };
    ibSubData.pSysMem = (char*)edgefillIB;
    if (g_pd3dDevice->CreateBuffer(&ibBufferDesc, &ibSubData, &g_IndexBufferEdgefill) != S_OK)
    {
        return;
    }

    // Wireframe vertex buffer will be grown dynamically.
    g_VertexBufferWireframe = nullptr;
}

struct TangentData
{
    Vec3 Normal;
    Vec3 Tangent;
    Vec3 Bitangent;
    float TotalWeight;
};

void GenerateTangentSpace()
{
    // A note on these tangent space vectors, as they're not like
    // the standard tangent spaces we generate for use with normal maps in gamedev.
    //
    // Here, the three vectors are in actual object space - the same space as
    // the vertex positions. Their lengths have "physical" meaning!
    // 
    // The normal vector's length is how far between the bottom of the heightfield
    // and the top of the heightfield, in actual meters. It is assumed that
    // the vertex position is where the 0.5 heightfield value is.
    //
    // If you took the texture mapped to a specific triangle and "unrolled it"
    // so the full 0.0-1.0 UV range was laid out in a flat plane extending the triangle,
    // the size of that plane in meters is how long the tangent and bitangent vectors are.
    // The tangent vector points along the U axis (i.e. the line of constant V),
    // and the bitangent vector points along the V axis (i.e. the line of contant U).
    // This means in general they are NOT at right angles to each other,
    // nor are they they same length, though they are often close to it.
    //
    // For all the above reasons, we can't just use off-the-shelf tangent-space
    // generators like MikkTSpace - though it should be possible to modify them
    // to NOT normalize the vectors.
    //
    // Additonally, it means that using standard tangent-space compression methods
    // such as octahedral maps and "Doom angles" won't work very well. One option
    // (which is on my TODO list) is to store the normalized vectors as octahedral
    // maps along with a separate length value.

    TangentData *tangentData = new TangentData [g_NumVerts];
    for ( int i = 0; i < g_NumVerts; i++ )
    {
        tangentData[i].Normal    = Vec3::zero;
        tangentData[i].Tangent   = Vec3::zero;
        tangentData[i].Bitangent = Vec3::zero;
        tangentData[i].TotalWeight = 0.0f;
    }

    UINT *curIndex = g_IndexData;
    for ( int triNum = 0; triNum < g_NumTris; triNum++ )
    {
        int index0 = *curIndex++;
        int index1 = *curIndex++;
        int index2 = *curIndex++;

        VertexBufferStruct *v0 = g_VertexData + index0;
        VertexBufferStruct *v1 = g_VertexData + index1;
        VertexBufferStruct *v2 = g_VertexData + index2;
        TangentData *t0 = tangentData + index0;
        TangentData *t1 = tangentData + index1;
        TangentData *t2 = tangentData + index2;

        // This is adapted from InitTriInfo() in MikkTSpace.c,
        // but without the normalisation steps or the robustness!

        Vec3 pos10 = v1->Pos - v0->Pos;
        Vec3 pos20 = v2->Pos - v0->Pos;
        Vec3 pos21 = v2->Pos - v1->Pos;
        Vec3 tex10 = v1->TexCoord - v0->TexCoord;
        Vec3 tex20 = v2->TexCoord - v0->TexCoord;

        float texArea = tex10.x * tex20.y - tex10.y * tex20.x;
        float texAreaAbs = fabsf ( texArea );

        // question - are these actually swapped?
        Vec3 tangent   = pos10 *  tex20.y + pos20 * -tex10.y;
        Vec3 bitangent = pos10 * -tex20.x + pos20 *  tex10.x;
        Vec3 normal = pos10.Cross ( pos20 );
        float triArea = normal.GetLength();

        ASSERT ( triArea > 0.0f );
        ASSERT ( texAreaAbs > 0.0f );

        normal *= 1.0f / triArea;
        tangent *= 1.0f / texAreaAbs;
        bitangent *= 1.0f / texAreaAbs;

        // Weight the contributions by angle of the vertex.
        float pos10Len = pos10.GetLength();
        float pos20Len = pos20.GetLength();
        float pos21Len = pos21.GetLength();
        float vert0Angle = acosf ( (  pos10 ).Dot (  pos20 ) / ( pos10Len * pos20Len ) );
        float vert1Angle = acosf ( ( -pos10 ).Dot (  pos21 ) / ( pos10Len * pos21Len ) );
        float vert2Angle = acosf ( ( -pos21 ).Dot ( -pos20 ) / ( pos21Len * pos20Len ) );
        ASSERT ( fabsf ( vert0Angle + vert1Angle + vert2Angle - PI ) < 0.001f ); // add up to 180 of course!

        t0->TotalWeight += vert0Angle;
        t0->Normal    += normal    * vert0Angle;
        t0->Tangent   += tangent   * vert0Angle;
        t0->Bitangent += bitangent * vert0Angle;

        t1->TotalWeight += vert1Angle;
        t1->Normal    += normal    * vert1Angle;
        t1->Tangent   += tangent   * vert1Angle;
        t1->Bitangent += bitangent * vert1Angle;

        t2->TotalWeight += vert2Angle;
        t2->Normal    += normal    * vert2Angle;
        t2->Tangent   += tangent   * vert2Angle;
        t2->Bitangent += bitangent * vert2Angle;
    }

    for ( int i = 0; i < g_NumVerts; i++ )
    {
        float totalWeightRcp = 1.0f;
        if ( tangentData[i].TotalWeight > 0.0f )
        {
            totalWeightRcp = 1.0f / tangentData[i].TotalWeight;
        }
        // Note that we do NOT use these computed normals, because we don't know how to smooth across seam edges.
        // TODO: this could be added - elsewhere we compute vertex proxmity connections.
        //g_VertexData[i].Normal    = tangentData[i].Normal    * totalWeightRcp;
        g_VertexData[i].Tangent   = tangentData[i].Tangent   * totalWeightRcp;
        g_VertexData[i].Bitangent = tangentData[i].Bitangent * totalWeightRcp;
    }
}


enum ImageFileType
{
    IFT_JPG,
    IFT_PNG,
};

bool CreateTextureAndNormalMapFromImage ( char const *filename, ImageFileType fileType,
                                          ID3D11Texture2D** texturePtr, ID3D11ShaderResourceView** srvPtr,
                                          ID3D11Texture2D** textureNormalPtr, ID3D11ShaderResourceView** srvNormalPtr,
                                          float contrast = 1.0f, float normalScale = 1.0f, int repeatCount = 1 )
{
    int width;
    int height;
    unsigned char* textureData;

    bool useStbFree = false;
    switch (fileType)
    {
    case IFT_JPG:
    case IFT_PNG:
    {
        int numComponents;
        // The 4 here means we request 4 components in the result - RGBA.
        // The return value n is how many compoenets we got back. It better be 4!
        textureData = stbi_load(filename, &width, &height, &numComponents, 4);
        if (!textureData)
        {
            ASSERT ( false );
            return false;
        }
        useStbFree = true;
        break;
    }
    default:
        ASSERT ( false );
        return false;
        break;
    }

    if ( contrast != 1.0f )
    {
        float scale = contrast;
        float bias = 128.0f - 128.0f * contrast;
        if ( contrast < 0.0f )
        {
            // Auto-contrast - finds the brightest/darkest and stretches them to 0-1.
            float darkest = 255.0f;
            float brightest = 0.0f;
            unsigned char *temp = textureData;
            for ( int i = 0; i < width * height; i++ )
            {
                for ( int j = 0; j < 3; j++ )
                {
                    float val = *temp++;
                    darkest = Min ( darkest, val );
                    brightest = Max ( brightest, val );
                }
                temp++; // skip alpha.
            }
            scale = 255.0f / ( brightest - darkest );
            bias = -darkest / scale;
        }

        unsigned char *temp = textureData;
        for ( int i = 0; i < width * height * 4; i++ )
        {
            float val = *temp;
            val = val * scale + bias;
            val = Clamp ( val, 0.0f, 255.0f );
            *temp++ = (unsigned char)floorf ( val + 0.5f );
        }
    }

    if ( repeatCount > 1 )
    {
        uint32_t* tempTextureData = (uint32_t*) malloc ( width * height * 4 );
        memcpy ( tempTextureData, textureData, width * height * 4 );

        uint32_t* dst = (uint32_t*)textureData;
        for ( int y = 0; y < height; y++ )
        {
            int srcY = (y * repeatCount) % height;
            for ( int x = 0; x < width; x++ )
            {
                int srcX = (x * repeatCount) % width;
                *dst++ = tempTextureData [ srcX + srcY * width ];
            }
        }

        free ( tempTextureData );
    }

    D3D11_TEXTURE2D_DESC tex2DDesc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R8G8B8A8_UNORM, width, height, 1, 1, D3D11_BIND_SHADER_RESOURCE);
    D3D11_SUBRESOURCE_DATA texSubData = { 0 };
    texSubData.pSysMem = textureData;
    texSubData.SysMemPitch = sizeof(char) * width * 4; // RGBA, 8 bits each.
    if (g_pd3dDevice->CreateTexture2D(&tex2DDesc, &texSubData, texturePtr) != S_OK)
    {
        ASSERT ( false );
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURE2D, tex2DDesc.Format, 0, 1);
    if (g_pd3dDevice->CreateShaderResourceView(*texturePtr, &srvDesc, srvPtr) != S_OK)
    {
        ASSERT ( false );
        return false;
    }

    if ( textureNormalPtr != nullptr )
    {
        ASSERT ( srvNormalPtr != nullptr );

        uint32_t* normalMap = (uint32_t*) malloc ( width * height * sizeof(uint32_t) );
        uint32_t* dst = normalMap;

        int kernelSize = 1;
        if ( width >= 1024 )
        {
            // Tend to need to grow the kernel for very high-rez surfaces.
            kernelSize = width / 1024;
        }

        for ( int y = 0; y < height; y++ )
        {
            int yUp = y - kernelSize;
            int yDn = y + kernelSize;
            if ( yUp < 0       ) { yUp += height; }
            if ( yDn >= height ) { yDn -= height; }
            for ( int x = 0; x < width; x++ )
            {
                int xLt = x - kernelSize;
                int xRt = x + kernelSize;
                if ( xLt < 0      ) { xLt += width; }
                if ( xRt >= width ) { xRt -= width; }
                float hUp = (float)textureData [ ( ( yUp * width ) + x   ) * 4 ];
                float hDn = (float)textureData [ ( ( yDn * width ) + x   ) * 4 ];
                float hLt = (float)textureData [ ( ( y   * width ) + xLt ) * 4 ];
                float hRt = (float)textureData [ ( ( y   * width ) + xRt ) * 4 ];
                Dir normal;
                normal.x = ( hLt - hRt );
                normal.y = ( hUp - hDn );
                normal.z = 127.0f * (float)kernelSize / normalScale;
                normal = normal.GetNormalise();
                // Since this is from a heightfield, there's lots of clever ways we could encode this.
                // But for simplicty, snorm8 is just fine.
                int8_t red = (int8_t)floorf ( 0.5f + Clamp ( normal.x * 127.0f, -127.0f, 127.0f ) );
                int8_t grn = (int8_t)floorf ( 0.5f + Clamp ( normal.y * 127.0f, -127.0f, 127.0f ) );
                int8_t blu = (int8_t)floorf ( 0.5f + Clamp ( normal.z * 127.0f, -127.0f, 127.0f ) );
                // Writing DWORDs, so remember that bytes RGBA are in a DWORD as ABGR
                uint32_t dstPixel = ( ( (uint32_t)red & 0xff ) << 0 ) | ( ( (uint32_t)grn & 0xff ) << 8 ) | ( ( (uint32_t)blu & 0xff ) << 16 );
                *dst++ = dstPixel;
            }
        }

        tex2DDesc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R8G8B8A8_SNORM, width, height, 1, 1, D3D11_BIND_SHADER_RESOURCE);
        texSubData = { 0 };
        texSubData.pSysMem = normalMap;
        texSubData.SysMemPitch = sizeof(char) * width * 4; // RGBA, 8 bits each.
        if (g_pd3dDevice->CreateTexture2D(&tex2DDesc, &texSubData, textureNormalPtr) != S_OK)
        {
            ASSERT ( false );
            return false;
        }

        srvDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURE2D, tex2DDesc.Format, 0, 1);
        if (g_pd3dDevice->CreateShaderResourceView(*textureNormalPtr, &srvDesc, srvNormalPtr) != S_OK)
        {
            ASSERT ( false );
            return false;
        }

        free ( normalMap );
    }
    else
    {
        ASSERT ( srvNormalPtr == nullptr );
    }

    // Pedantic difference.
    if ( useStbFree )
    {
        stbi_image_free(textureData);
    }
    else
    {
        free ( textureData );
    }

    return true;
}

bool CreateTextureFromImage ( char const *filename, ImageFileType fileType, ID3D11Texture2D** texturePtr, ID3D11ShaderResourceView** srvPtr, float contrast = 1.0f, int repeatCount = 1 )
{
    return CreateTextureAndNormalMapFromImage ( filename, fileType, texturePtr, srvPtr, nullptr, nullptr, contrast, 1.0f, repeatCount );
}

bool CreateRenderTarget ( int width, int height, DXGI_FORMAT format, ID3D11Texture2D** texturePtr, ID3D11ShaderResourceView** srvPtr, ID3D11RenderTargetView **rtvPtr )
{
    D3D11_TEXTURE2D_DESC tex2DDesc = CD3D11_TEXTURE2D_DESC(format, width, height, 1, 1, D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET, D3D11_USAGE_DEFAULT);
    D3D11_SUBRESOURCE_DATA texSubData = { 0 };
    if (g_pd3dDevice->CreateTexture2D(&tex2DDesc, nullptr, texturePtr) != S_OK)
    {
        ASSERT ( false );
        return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURE2D, format, 0, 1);
    if (g_pd3dDevice->CreateShaderResourceView(*texturePtr, &srvDesc, srvPtr) != S_OK)
    {
        ASSERT ( false );
        return false;
    }

    D3D11_RENDER_TARGET_VIEW_DESC rtvDesc;
    rtvDesc.Format = format;
    rtvDesc.ViewDimension = D3D11_RTV_DIMENSION_TEXTURE2D;
    rtvDesc.Texture2D.MipSlice = 0;
    if(g_pd3dDevice->CreateRenderTargetView(*texturePtr, &rtvDesc, rtvPtr) != S_OK)
    {
        ASSERT ( false );
        return false;
    }

    return true;
}

void CreateTextures()
{
    SAFE_RELEASE (g_TextureAlbedo);
    SAFE_RELEASE (g_TextureHeight);
    SAFE_RELEASE (g_TextureNormal);
    SAFE_RELEASE (g_TextureAlbedoSRV);
    SAFE_RELEASE (g_TextureHeightSRV);
    SAFE_RELEASE (g_TextureNormalSRV);
    SAFE_RELEASE (g_TextureSurfaceFromObjectTemp0);
    SAFE_RELEASE (g_TextureSurfaceFromObjectTemp1);
    SAFE_RELEASE (g_TextureSurfaceFromObjectTemp2);
    SAFE_RELEASE (g_TextureSurfaceFromObjectTemp3);
    SAFE_RELEASE (g_TextureSurfaceFromObject0);
    SAFE_RELEASE (g_TextureSurfaceFromObject1);
    SAFE_RELEASE (g_TextureSurfaceFromObject2);
    SAFE_RELEASE (g_TextureSurfaceFromObject3);
    SAFE_RELEASE (g_TextureTeleportMap);
    SAFE_RELEASE (g_TextureEdgefillMap);
    SAFE_RELEASE (g_TextureSurfaceFromObjectTemp0SRV);
    SAFE_RELEASE (g_TextureSurfaceFromObjectTemp1SRV);
    SAFE_RELEASE (g_TextureSurfaceFromObjectTemp2SRV);
    SAFE_RELEASE (g_TextureSurfaceFromObjectTemp3SRV);
    SAFE_RELEASE (g_TextureSurfaceFromObject0SRV);
    SAFE_RELEASE (g_TextureSurfaceFromObject1SRV);
    SAFE_RELEASE (g_TextureSurfaceFromObject2SRV);
    SAFE_RELEASE (g_TextureSurfaceFromObject3SRV);
    SAFE_RELEASE (g_TextureTeleportMapSRV);
    SAFE_RELEASE (g_TextureEdgefillMapSRV);
    SAFE_RELEASE (g_TextureSurfaceFromObjectTemp0RTV);
    SAFE_RELEASE (g_TextureSurfaceFromObjectTemp1RTV);
    SAFE_RELEASE (g_TextureSurfaceFromObjectTemp2RTV);
    SAFE_RELEASE (g_TextureSurfaceFromObjectTemp3RTV);
    SAFE_RELEASE (g_TextureSurfaceFromObject0RTV);
    SAFE_RELEASE (g_TextureSurfaceFromObject1RTV);
    SAFE_RELEASE (g_TextureSurfaceFromObject2RTV);
    SAFE_RELEASE (g_TextureSurfaceFromObject3RTV);

    g_TextureSet_Current = g_TextureSet;

    switch ( g_TextureSet )
    {
    case 0:
        CreateTextureFromImage ( "assets\\roof_3_4k.blend\\textures\\roof_3_diff_4k.jpg", IFT_JPG, &g_TextureAlbedo, &g_TextureAlbedoSRV, 1.0f, 1 );
        CreateTextureAndNormalMapFromImage ( "assets\\roof_3_4k.blend\\textures\\roof_3_disp_4k.png", IFT_JPG, &g_TextureHeight, &g_TextureHeightSRV, &g_TextureNormal, &g_TextureNormalSRV, 1.0f, 8.0f, 1 );
        break;
    case 1:
        CreateTextureFromImage ( "assets\\aerial_rocks_02_4k.blend\\textures\\aerial_rocks_02_diff_4k.jpg", IFT_JPG, &g_TextureAlbedo, &g_TextureAlbedoSRV, 1.0f, 4 );
        CreateTextureAndNormalMapFromImage ( "assets\\aerial_rocks_02_4k.blend\\textures\\aerial_rocks_02_disp_4k.png", IFT_JPG, &g_TextureHeight, &g_TextureHeightSRV, &g_TextureNormal, &g_TextureNormalSRV, 1.0f, 8.0f, 4 );
        break;
    case 2:
    default:
        CreateTextureFromImage ( "assets\\Textures_a.png", IFT_PNG, &g_TextureAlbedo, &g_TextureAlbedoSRV );
        CreateTextureAndNormalMapFromImage ( "assets\\Textures_h.png", IFT_PNG, &g_TextureHeight, &g_TextureHeightSRV, &g_TextureNormal, &g_TextureNormalSRV, 1.0f, 0.125f, 1 );
        break;
    }

    CreateRenderTarget ( g_SurfaceFromObjectTextureSize, g_SurfaceFromObjectTextureSize, DXGI_FORMAT_R32G32B32A32_FLOAT, &g_TextureSurfaceFromObjectTemp0, &g_TextureSurfaceFromObjectTemp0SRV, &g_TextureSurfaceFromObjectTemp0RTV );
    CreateRenderTarget ( g_SurfaceFromObjectTextureSize, g_SurfaceFromObjectTextureSize, DXGI_FORMAT_R32G32B32A32_FLOAT, &g_TextureSurfaceFromObjectTemp1, &g_TextureSurfaceFromObjectTemp1SRV, &g_TextureSurfaceFromObjectTemp1RTV );
    CreateRenderTarget ( g_SurfaceFromObjectTextureSize, g_SurfaceFromObjectTextureSize, DXGI_FORMAT_R32G32B32A32_FLOAT, &g_TextureSurfaceFromObjectTemp2, &g_TextureSurfaceFromObjectTemp2SRV, &g_TextureSurfaceFromObjectTemp2RTV );
    CreateRenderTarget ( g_SurfaceFromObjectTextureSize, g_SurfaceFromObjectTextureSize, DXGI_FORMAT_R32G32B32A32_FLOAT, &g_TextureSurfaceFromObjectTemp3, &g_TextureSurfaceFromObjectTemp3SRV, &g_TextureSurfaceFromObjectTemp3RTV );
    CreateRenderTarget ( g_SurfaceFromObjectTextureSize, g_SurfaceFromObjectTextureSize, DXGI_FORMAT_R32G32B32A32_FLOAT, &g_TextureSurfaceFromObject0, &g_TextureSurfaceFromObject0SRV, &g_TextureSurfaceFromObject0RTV );
    CreateRenderTarget ( g_SurfaceFromObjectTextureSize, g_SurfaceFromObjectTextureSize, DXGI_FORMAT_R32G32B32A32_FLOAT, &g_TextureSurfaceFromObject1, &g_TextureSurfaceFromObject1SRV, &g_TextureSurfaceFromObject1RTV );
    CreateRenderTarget ( g_SurfaceFromObjectTextureSize, g_SurfaceFromObjectTextureSize, DXGI_FORMAT_R32G32B32A32_FLOAT, &g_TextureSurfaceFromObject2, &g_TextureSurfaceFromObject2SRV, &g_TextureSurfaceFromObject2RTV );
    CreateRenderTarget ( g_SurfaceFromObjectTextureSize, g_SurfaceFromObjectTextureSize, DXGI_FORMAT_R32G32B32A32_FLOAT, &g_TextureSurfaceFromObject3, &g_TextureSurfaceFromObject3SRV, &g_TextureSurfaceFromObject3RTV );

    g_SurfaceFromObjectTextureSize_Current = g_SurfaceFromObjectTextureSize;
}

struct TeleportEdgefillData
{
    bool surfaceWarpPresent;

    float edgefillDistance;
    Vec2 edgefillUV;

    float teleportDistance;
    Vec2 teleportUV;
};

void CreateTeleportEdgefill()
{
    SAFE_RELEASE ( g_TextureTeleportMap );
    SAFE_RELEASE ( g_TextureTeleportMapSRV );
    SAFE_RELEASE ( g_TextureEdgefillMap );
    SAFE_RELEASE ( g_TextureEdgefillMapSRV );

    // Generate these two with a multi-step process.

    // 1. Clear SurfaceFromObjectN with INF.
    // 2. Do a render of the distortion pass to the SurfaceFromObject0
    // 3. Set up an occupancy map - start cleared.
    // 4. Read the texture to see which texels are actually filled by the distortion pass.
    // 5. Find all the matching mesh edges.
    // 6. Walk the edges, setting up two-way teleports.
    // 7. Flood-fill edgefill and teleport maps.
    // 8. Bake teleport into a texture:
    // 9. Bake edgefill into another texture.


    // So...
    // 1. Clear SurfaceFromObjectN with INF.

    // I use INF because it doesn't signal annoyingly like NANs, but you can still test for it pretty easily.
    float inf = 1.0f;
    inf = inf / ( inf - inf ); // you have to trick the compiler into doing a divide by zero.
    Vec4 infVec ( inf, inf, inf, inf );
    g_pd3dDeviceContext->ClearRenderTargetView(g_TextureSurfaceFromObjectTemp0RTV, infVec.AsFloatPtr());
    g_pd3dDeviceContext->ClearRenderTargetView(g_TextureSurfaceFromObjectTemp1RTV, infVec.AsFloatPtr());
    g_pd3dDeviceContext->ClearRenderTargetView(g_TextureSurfaceFromObjectTemp2RTV, infVec.AsFloatPtr());
    g_pd3dDeviceContext->ClearRenderTargetView(g_TextureSurfaceFromObjectTemp3RTV, infVec.AsFloatPtr());

    // 2. Do a render of the distortion pass to the SurfaceFromObject0
    // We just need to set up some non-bogus and non-inf numbers in the constant buffer. The values won't actually be used.
    g_ConstantBufferData.projectionFromCameraMatrix = Mat44::identity;
    g_ConstantBufferData.cameraFromObjectMatrix = Mat44::identity;
    g_ConstantBufferData.objectFromCameraMatrix = Mat44::identity;

    g_ConstantBufferData.SunDirInObject = Dir::zero;

    for ( int boneNum = 0; boneNum < 4; boneNum++ )
    {
        g_ConstantBufferData.BoneFromObject[boneNum] = Mat44::identity;
    }

    RenderDistortionPass();

    // 3. Set up an occupancy map - start cleared.

    TeleportEdgefillData** teleportEdgefillMap = new TeleportEdgefillData*[g_SurfaceFromObjectTextureSize];
    for ( int i = 0; i < g_SurfaceFromObjectTextureSize; i++ )
    {
        teleportEdgefillMap[i] = new TeleportEdgefillData[g_SurfaceFromObjectTextureSize];
    }

    // 4. Read the texture to see which texels are actually filled by the distortion pass.

    // Because the texture does not have read-access flags for performance, we create & copy through a resource that does.
    ID3D11Texture2D *cpuAccessTexture;
    D3D11_TEXTURE2D_DESC tex2DDesc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R32G32B32A32_FLOAT, g_SurfaceFromObjectTextureSize, g_SurfaceFromObjectTextureSize, 1, 1, 0, D3D11_USAGE_STAGING, D3D11_CPU_ACCESS_READ);
    D3D11_SUBRESOURCE_DATA texSubData = { 0 };
    HRESULT res = g_pd3dDevice->CreateTexture2D(&tex2DDesc, nullptr, &cpuAccessTexture);
    ASSERT ( res == S_OK );
    g_pd3dDeviceContext->CopyResource(cpuAccessTexture, g_TextureSurfaceFromObjectTemp0);

    D3D11_MAPPED_SUBRESOURCE mappedSubRes = {0};
    res = g_pd3dDeviceContext->Map(cpuAccessTexture, 0, D3D11_MAP_READ, 0, &mappedSubRes);
    ASSERT ( res == S_OK );
    ASSERT ( mappedSubRes.RowPitch == g_SurfaceFromObjectTextureSize * sizeof(float) * 4 );
    Vec4* srcSurfaceFromObject = (Vec4*)mappedSubRes.pData;
    for ( int y = 0; y < g_SurfaceFromObjectTextureSize; y++ )
    {
        for ( int x = 0; x < g_SurfaceFromObjectTextureSize; x++ )
        {
            // Init.
            teleportEdgefillMap[y][x].surfaceWarpPresent = false;
            teleportEdgefillMap[y][x].edgefillDistance = FLT_MAX;
            teleportEdgefillMap[y][x].edgefillUV = Vec2 ( -100.0f, -100.0f );
            teleportEdgefillMap[y][x].teleportDistance = FLT_MAX;
            teleportEdgefillMap[y][x].teleportUV = Vec2 ( -100.0f, -100.0f );

            // Did this get set by the distortion render pass?
            float val = srcSurfaceFromObject->x;
            if ( val <= FLT_MAX )
            {
                teleportEdgefillMap[y][x].surfaceWarpPresent = true;
                teleportEdgefillMap[y][x].teleportDistance = -FLT_MAX;
            }
            srcSurfaceFromObject++;
        }
    }
    g_pd3dDeviceContext->Unmap(cpuAccessTexture, 0);
    cpuAccessTexture->Release();

    // 5. Find all the matching mesh edges.
    // "Matching" means they don'thave an actual shared edge in the mesh, but they have another edge whose vertices share positions.
    // I should also check they share normals, and it's not a sharp crease... maybe later.

    // So first set up a circular list of all coincident verts.
    UINT* vertexProx = new UINT[g_NumVerts];
    for ( int i = 0; i < g_NumVerts; i++ )
    {
        VertexBufferStruct *curVert = &(g_VertexData[i]);
        vertexProx[i] = (UINT)i;
        for ( int j = 0; j < i; j++ )
        {
            VertexBufferStruct *otherVert = &(g_VertexData[j]);
            Vec3 delta = curVert->Pos - otherVert->Pos;
            if ( delta.GetLengthSq() < 0.001f * 0.001f )
            {
                // Insert into the linked list.
                ASSERT ( vertexProx[i] == (unsigned)i );
                int nextVert = vertexProx[j];
                vertexProx[j] = i;
                vertexProx[i] = nextVert;
                break;
            }
        }
    }

    // 6. Walk the edges, setting up two-way teleports.
    // 
    // Now find triangle edges that do not have matching opposite edges in another triangle.
    // i.e. they may be one half of a seam, or they may just be a long edge.

    // The only reason to store these edges is for the wireframe debug view.
    if ( g_EdgeList != nullptr )
    {
        delete g_EdgeList;
    }
    g_EdgeList = new EdgeStruct[g_NumTris * 3];
    g_EdgeListCount = 0;

    for ( int triNum = 0; triNum < g_NumTris; triNum++ )
    {
        UINT v1 = g_IndexData[triNum * 3 + 2];
        for ( int edge = 0; edge < 3; edge++ )
        {
            UINT v2 = g_IndexData[triNum * 3 + edge];

            // Look for v2, v1 in another triangle.
            for ( int triNum2 = 0; triNum2 < g_NumTris; triNum2++ )
            {
                UINT other_v1 = g_IndexData[triNum2 * 3 + 2];
                for ( int otherEdge = 0; otherEdge < 3; otherEdge++ )
                {
                    UINT other_v2 = g_IndexData[triNum2 * 3 + otherEdge];

                    if ( ( v1 == other_v2 ) && ( v2 == other_v1 ) )
                    {
                        // Not a lone edge - found a match.
                        goto edge_handled;
                    }

                    other_v1 = other_v2;
                }
            }

            // Got here, therefore this is a lone edge.
            // However, it might have an edge in coincident vertices.
            // Note you also need to handle the case where it's a "V" edge,
            // where the two edges do share one verte, but the others are prox.
            // 
            // For each of the pair of proximal verts, see if that (reverse) edge exists.
            // Again, note that we DO want to check the case of v1 not having any prox vertices.
            UINT v2Prox = ~0u;
            UINT v1Prox = v1;
            while ( true )
            {
                v2Prox = v2;
                while ( true )
                {
                    for ( int proxTriNum = 0; proxTriNum < g_NumTris; proxTriNum++ )
                    {
                        if ( ( v2Prox == g_IndexData[proxTriNum * 3 + 0] ) && ( v1Prox == g_IndexData[proxTriNum * 3 + 1] ) )
                        {
                            goto found_prox_edge;
                        }
                        if ( ( v2Prox == g_IndexData[proxTriNum * 3 + 1] ) && ( v1Prox == g_IndexData[proxTriNum * 3 + 2] ) )
                        {
                            goto found_prox_edge;
                        }
                        if ( ( v2Prox == g_IndexData[proxTriNum * 3 + 2] ) && ( v1Prox == g_IndexData[proxTriNum * 3 + 0] ) )
                        {
                            goto found_prox_edge;
                        }
                    }

                    v2Prox = vertexProx[v2Prox];
                    if ( v2Prox == v2 )
                    {
                        break;
                    }
                }

                v1Prox = vertexProx[v1Prox];
                if ( v1Prox == v1 )
                {
                    break;
                }
            }

            // No matching proximal edges found - this is a "naked" edge. Not sure what to do about that!
            ASSERTONCE ( false );
            goto edge_handled;

            found_prox_edge:;
            // Yes! The matching edge goes from v2Prox to v1Prox.
            // Now walk both edges, writing teleport data from one to the other.
            // Note - the UV destination of the teleport is not used to do
            // anything but pick up a new SurfaceFromObject matrix, so as long as that
            // is somewhat smooth, getting close should give the right answer.
            //
            // Because multiple edges can want to write to the same target texel,
            // we store the distance to the edge. Closest distance wins.
            // And we're going to flood-fill later as well. Together, these mean
            // instead of some complex Brezenham line draw, we just fill the
            // bounding boxes and let the distance check sort things out.
            //
            // Subtlety here! You might think that instead of teleporting the
            // UV coords to the new location and then sampling the SurfaceFromObject
            // data from there, why don't we just copy the data directly
            // (i.e. using the edgefill mechanism). And that would work IF we were
            // point-sampling the SurfaceFromObject matrix. But we want to make it
            // a fairly small texture and use linear interpolation to do a lot of
            // the heavy lifting. So putting two completely unrelated fields
            // next to each other won't make the lerp do anything sensible.
            // That's why the UVs need to be teleported, and then re-sampled in
            // the new location.

            // The only reason to store these edges is for the wireframe debug view.
            EdgeStruct newEdge;
            newEdge.v1 = v1;
            newEdge.v2 = v2;
            newEdge.v1Prox = v1Prox;
            newEdge.v2Prox = v2Prox;
            g_EdgeList[g_EdgeListCount++] = newEdge;

            for ( int direction = 0; direction < 2; direction++ )
            {
                // Filling the box around v2Prox/v1Prox with teleports to v1/v2
                VertexBufferStruct *vert1Src = &(g_VertexData[v1]);
                VertexBufferStruct *vert2Src = &(g_VertexData[v2]);
                VertexBufferStruct *vert1Dst = &(g_VertexData[v1Prox]);
                VertexBufferStruct *vert2Dst = &(g_VertexData[v2Prox]);

                {
                    // To make sure the SDF has full range, make sure we fill a bit extra.
                    int extraTexels = 5;
                    int uStart = Clamp ( (int)floorf ( Min ( vert1Dst->TexCoord.x, vert2Dst->TexCoord.x ) * g_SurfaceFromObjectTextureSize ) - extraTexels, 0, g_SurfaceFromObjectTextureSize - 1 );
                    int vStart = Clamp ( (int)floorf ( Min ( vert1Dst->TexCoord.y, vert2Dst->TexCoord.y ) * g_SurfaceFromObjectTextureSize ) - extraTexels, 0, g_SurfaceFromObjectTextureSize - 1 );
                    int uEnd   = Clamp ( (int)floorf ( Max ( vert1Dst->TexCoord.x, vert2Dst->TexCoord.x ) * g_SurfaceFromObjectTextureSize ) + extraTexels, 0, g_SurfaceFromObjectTextureSize - 1 );
                    int vEnd   = Clamp ( (int)floorf ( Max ( vert1Dst->TexCoord.y, vert2Dst->TexCoord.y ) * g_SurfaceFromObjectTextureSize ) + extraTexels, 0, g_SurfaceFromObjectTextureSize - 1 );
                    Vec2 vert1DstUV = Vec2 ( vert1Dst->TexCoord.x, vert1Dst->TexCoord.y );
                    Vec2 vert2DstUV = Vec2 ( vert2Dst->TexCoord.x, vert2Dst->TexCoord.y );
                    Vec2 vertDstUVDelta = vert2DstUV - vert1DstUV;
                    float vertDstUVDeltaLengthSq = vertDstUVDelta.GetLengthSq();
                    for ( int uInt = uStart; uInt <= uEnd; uInt++ )
                    {
                        for ( int vInt = vStart; vInt <= vEnd; vInt++ )
                        {
                            // The middle of the texel.
                            float u = ( (float)uInt + 0.5f ) / (float)g_SurfaceFromObjectTextureSize;
                            float v = ( (float)vInt + 0.5f ) / (float)g_SurfaceFromObjectTextureSize;
                            Vec2 uv ( u, v );

                            // ...and where is that along the edge from vert1Dst to vert2Dst, as a fraction 0...1
                            Vec2 uvThisVert1Delta = uv - vert1DstUV;
                            float lambda = vertDstUVDelta.Dot ( uvThisVert1Delta ) / vertDstUVDeltaLengthSq;
                            lambda = Clamp ( lambda, 0.0f, 1.0f );

                            // So now that's a warp to here.
                            // Subtlety here is we are warping to the nearest point on the matching edge.
                            // A better thing to do would be to warp to the equivalent place on the other side of the edge,
                            // i.e. the starting point is not ON the edge, it's slightly past the edge,
                            // so it would be good to teleport to the matching place on the mesh. However,
                            // this is tricky to do, and there is no guarantee that the destination would not immediately need
                            // another teleport. In practice this seems to work fine.
                            Vec3 teleportDest = vert1Src->TexCoord + ( vert2Src->TexCoord - vert1Src->TexCoord ) * lambda;

                            // ...and in texels it is this far away from the edge...
                            Vec2 nearestUV = vert1DstUV + ( vertDstUVDelta ) * lambda;
                            Vec2 vectorToNearestPoint = nearestUV - uv;
                            float distance = vectorToNearestPoint.GetLength();
                            if ( vectorToNearestPoint.x * vertDstUVDelta.y < vectorToNearestPoint.y * vertDstUVDelta.x ) // sign of the 2D cross-product.
                            {
                                // Inside the mesh.
                                distance = -distance;
                            }
                            float curDistance = teleportEdgefillMap[vInt][uInt].teleportDistance;

                            bool replace = false;
                            if ( distance < 0.0f )
                            {
                                replace = distance > curDistance;
                                if ( distance < -2.0f / (float)g_SurfaceFromObjectTextureSize )
                                {
                                    // This is way inside the mesh. For easier visualisation,
                                    // use the current UV, not the teleport destination one.
                                    teleportDest.x = u;
                                    teleportDest.y = v;
                                }
                            }
                            else
                            {
                                replace = distance < curDistance;
                            }

                            if ( replace )
                            {
                                teleportEdgefillMap[vInt][uInt].teleportDistance = distance;
                                teleportEdgefillMap[vInt][uInt].teleportUV = Vec2 ( teleportDest.x, teleportDest.y );
                            }
                        }
                    }
                }



                // Swap and do the other way.
                int temp = v1Prox;
                v1Prox = v2;
                v2 = temp;
                temp = v2Prox;
                v2Prox = v1;
                v1 = temp;
            }

            edge_handled:;

            v1 = v2;
        }
    }

    g_FloodFillTeleportEdgefill_Current = g_FloodFillTeleportEdgefill;
    if (g_FloodFillTeleportEdgefill)
    {
        // 7. Flood-fill edgefill and teleport maps.
        // This could obviously be done in a more efficient manner!
        int yDelta[8] = { -1, -1,  0,  1,  1,  1,  0, -1 };
        int xDelta[8] = {  0,  1,  1,  1,  0, -1, -1, -1 };
        float dist11 = sqrtf ( 2.0f ) / (float)g_SurfaceFromObjectTextureSize;
        float dist10 = 1.0f / (float)g_SurfaceFromObjectTextureSize;
        float distDelta[8] = { dist10, dist11, dist10, dist11, dist10, dist11, dist10, dist11 };

        bool moreToDo = true;
        while ( moreToDo )
        {
            moreToDo = false;
            for ( int y = 0; y < g_SurfaceFromObjectTextureSize; y++ )
            {
                for ( int x = 0; x < g_SurfaceFromObjectTextureSize; x++ )
                {
                    // Flood fill rules:
                    // if ( cell.surfaceWarpPresent ) && ( !neighbour.edgefill && !neighbour.surfaceWarpPresent ) { make neighbour edgefill from this cell }
                    // if ( cell.edgefill           ) && ( !neighbour.edgefill && !neighbour.surfaceWarpPresent ) { copy edgefill to neighbour }
                    // if ( cell.teleport           ) && ( !neighbour.teleport && !neighbour.surfaceWarpPresent ) { copy teleport to neighbour }
                    TeleportEdgefillData *center = &(teleportEdgefillMap[y][x]);
                    if ( center->surfaceWarpPresent || ( center->teleportDistance < FLT_MAX ) || ( center->edgefillDistance < FLT_MAX ) )
                    {
                        float u = ( (float)x + 0.5f ) / (float)g_SurfaceFromObjectTextureSize;
                        float v = ( (float)y + 0.5f ) / (float)g_SurfaceFromObjectTextureSize;
                        Vec2 uvCenter ( u, v );

                        float teleportTestDist = center->teleportDistance + dist11;
                        if ( center->teleportDistance < 0.0f )
                        {
                            teleportTestDist = center->teleportDistance - dist11;
                        }

                        float edgefillTestDist = center->edgefillDistance + dist11;
                        if ( center->surfaceWarpPresent )
                        {
                            edgefillTestDist = dist11;
                        }

                        for ( int neiNum = 0; neiNum < 8; neiNum++ )
                        {
                            int nx = x + xDelta[neiNum];
                            int ny = y + yDelta[neiNum];
                            if ( ( nx >= 0 ) && ( nx < g_SurfaceFromObjectTextureSize ) &&
                                 ( ny >= 0 ) && ( ny < g_SurfaceFromObjectTextureSize ) )
                            {
                                TeleportEdgefillData *neigh = &(teleportEdgefillMap[ny][nx]);
                                if ( !neigh->surfaceWarpPresent && ( neigh->edgefillDistance > edgefillTestDist ) )
                                {
                                    if ( center->surfaceWarpPresent )
                                    {
                                        // Edgefill from this texel.
                                        neigh->edgefillDistance = distDelta[neiNum];
                                        neigh->edgefillUV = uvCenter;
                                        moreToDo = true;
                                    }
                                    else if ( center->edgefillDistance < FLT_MAX )
                                    {
                                        // Copy edgefill from here.
                                        neigh->edgefillUV = center->edgefillUV;
                                        neigh->edgefillDistance = center->edgefillDistance + distDelta[neiNum];
                                        moreToDo = true;
                                    }
                                }

                                if ( teleportTestDist > 0.0f )
                                {
                                    if ( neigh->teleportDistance > teleportTestDist )
                                    {
                                        if ( center->teleportDistance < FLT_MAX )
                                        {
                                            // Copy teleport from here.
                                            neigh->teleportUV = center->teleportUV;
                                            neigh->teleportDistance = center->teleportDistance + distDelta[neiNum];
                                            moreToDo = true;
                                        }
                                    }
                                }
                                else
                                {
                                    if ( ( neigh->teleportDistance == FLT_MAX ) || ( neigh->teleportDistance < teleportTestDist ) )
                                    {
                                        // Copy teleport from here.
                                        neigh->teleportUV = center->teleportUV;
                                        neigh->teleportDistance = center->teleportDistance - distDelta[neiNum];
                                        moreToDo = true;
                                    }
                                }
                            }
                        }
                    }
                }
            }
        }
    }

    // 8. Bake teleport into a texture:

    // For now, just use a 3-float texture. It could be compressed down to 2*UNORM16 and a separate 8-bit SDF biased so that 128=0.0.
    Vec3 *finalTeleportMap = new Vec3 [g_SurfaceFromObjectTextureSize * g_SurfaceFromObjectTextureSize];
    Vec3 *dst = finalTeleportMap;
    for ( int y = 0; y < g_SurfaceFromObjectTextureSize; y++ )
    {
        for ( int x = 0; x < g_SurfaceFromObjectTextureSize; x++ )
        {
            float u = ( (float)x + 0.5f ) / (float)g_SurfaceFromObjectTextureSize;
            float v = ( (float)y + 0.5f ) / (float)g_SurfaceFromObjectTextureSize;
            Vec2 uvCenter ( u, v );

            // The teleport data is first read with bilinear sampling,
            // but only the SDF in the Z value is read.
            // If the filtered Z value is >0 then it is re-read with POINT
            // sampling (because there are discontinuities) and the XY values used as
            // the destination UV coordinates.
            //
            // It may be interesting to use multiple sets of teleport channels,
            // splitting the discontinuities across them, so that the UV coordinates
            // can be read with filtering, improving their precision, and requiring fewer
            // iterations around teleports. At the same time i would be interesting to
            // use a multi-channel SDF so that sharp corners are handled perfectly,
            // rather than being rounded off.

            float dist = teleportEdgefillMap[y][x].teleportDistance;
            dist *= 50.0f; // doesn't change the SDF==0 point, but makes visual debugging easier.
            if ( dist == FLT_MAX )
            {
                *dst++ = Vec3 ( 0.0f, 0.0f, 1000.0f );
            }
            else
            {
                *dst++ = Vec3 ( teleportEdgefillMap[y][x].teleportUV.x,
                                teleportEdgefillMap[y][x].teleportUV.y,
                                dist );
            }
        }
    }

    tex2DDesc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R32G32B32_FLOAT, g_SurfaceFromObjectTextureSize, g_SurfaceFromObjectTextureSize, 1, 1, D3D11_BIND_SHADER_RESOURCE);
    texSubData = { 0 };
    texSubData.pSysMem = finalTeleportMap;
    texSubData.SysMemPitch = g_SurfaceFromObjectTextureSize * sizeof(finalTeleportMap[0]);
    if (g_pd3dDevice->CreateTexture2D(&tex2DDesc, &texSubData, &g_TextureTeleportMap) != S_OK)
    {
        ASSERT ( false );
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURE2D, tex2DDesc.Format, 0, 1);
    if (g_pd3dDevice->CreateShaderResourceView(g_TextureTeleportMap, &srvDesc, &g_TextureTeleportMapSRV) != S_OK)
    {
        ASSERT ( false );
    }

    delete[] finalTeleportMap;

    // 9. Bake edgefill into another texture.

    // For now, just use a 3-float texture. It could be compressed down to 2*UNORM16
    Vec3 *finalEdgefillMap = new Vec3 [g_SurfaceFromObjectTextureSize * g_SurfaceFromObjectTextureSize];
    dst = finalEdgefillMap;
    for ( int y = 0; y < g_SurfaceFromObjectTextureSize; y++ )
    {
        for ( int x = 0; x < g_SurfaceFromObjectTextureSize; x++ )
        {
            float u = ( (float)x + 0.5f ) / (float)g_SurfaceFromObjectTextureSize;
            float v = ( (float)y + 0.5f ) / (float)g_SurfaceFromObjectTextureSize;
            Vec2 uvCenter ( u, v );

            float dist = teleportEdgefillMap[y][x].edgefillDistance;
            if ( teleportEdgefillMap[y][x].surfaceWarpPresent )
            {
                *dst++ = Vec3 ( u, v, 0.0f );
            }
            else if ( dist == FLT_MAX )
            {
                *dst++ = Vec3 ( 0.0f, 0.0f, 0.0f );
            }
            else
            {
                *dst++ = Vec3 ( teleportEdgefillMap[y][x].edgefillUV.x,
                                teleportEdgefillMap[y][x].edgefillUV.y,
                                dist );
            }
        }
    }

    tex2DDesc = CD3D11_TEXTURE2D_DESC(DXGI_FORMAT_R32G32B32_FLOAT, g_SurfaceFromObjectTextureSize, g_SurfaceFromObjectTextureSize, 1, 1, D3D11_BIND_SHADER_RESOURCE);
    texSubData = { 0 };
    texSubData.pSysMem = finalEdgefillMap;
    texSubData.SysMemPitch = g_SurfaceFromObjectTextureSize * sizeof(finalEdgefillMap[0]);
    if (g_pd3dDevice->CreateTexture2D(&tex2DDesc, &texSubData, &g_TextureEdgefillMap) != S_OK)
    {
        ASSERT ( false );
    }

    srvDesc = CD3D11_SHADER_RESOURCE_VIEW_DESC(D3D11_SRV_DIMENSION_TEXTURE2D, tex2DDesc.Format, 0, 1);
    if (g_pd3dDevice->CreateShaderResourceView(g_TextureEdgefillMap, &srvDesc, &g_TextureEdgefillMapSRV) != S_OK)
    {
        ASSERT ( false );
    }

    delete[] finalEdgefillMap;

    // Cleanup

    for ( int i = 0; i < g_SurfaceFromObjectTextureSize; i++ )
    {
        delete[] teleportEdgefillMap[i];
    }
    delete[] teleportEdgefillMap;
}

void CreateShaders()
{
    // All shaders share a constant buffer input.

    D3D11_BUFFER_DESC constantBufferDesc = CD3D11_BUFFER_DESC(sizeof(g_ConstantBufferData), D3D11_BIND_CONSTANT_BUFFER, D3D11_USAGE_DYNAMIC, D3D11_CPU_ACCESS_WRITE);
    D3D11_SUBRESOURCE_DATA constantSubData = { 0 };
    constantSubData.pSysMem = &g_ConstantBufferData;
    if (g_pd3dDevice->CreateBuffer(&constantBufferDesc, &constantSubData, &g_ConstantBuffer) != S_OK)
    {
        // If this fails, remember it needs to be a multiple of 16 bytes.
        ASSERT ( !"Constant buffer failed" );
        return;
    }

    g_Pipeline[PipelineState::Pipeline_Main].filename = L"PipelineMain.hlsl";
    g_Pipeline[PipelineState::Pipeline_Deform].filename = L"PipelineDeform.hlsl";
    g_Pipeline[PipelineState::Pipeline_Edgefill].filename = L"PipelineEdgefill.hlsl";
    g_Pipeline[PipelineState::Pipeline_Wireframe].filename = L"PipelineWireframe.hlsl";

    for ( int pipelineNum = 0; pipelineNum < PipelineState::Pipeline_COUNT; pipelineNum++)
    {
        PipelineState *pipeline = &g_Pipeline[pipelineNum];

        ID3DBlob* vertexShaderBlob = nullptr;
        ID3DBlob* pixelShaderBlob = nullptr;
        ID3DBlob* errorMessages = nullptr;

        if (D3DCompileFromFile(pipeline->filename, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "vs_main", "vs_5_0", 0, 0, &vertexShaderBlob, &errorMessages) != S_OK)
        {
            char const *errors = (char const *)errorMessages->GetBufferPointer();
            (void)errors;
            ASSERT ( false );
            return;
        }
        if (D3DCompileFromFile(pipeline->filename, nullptr, D3D_COMPILE_STANDARD_FILE_INCLUDE, "ps_main", "ps_5_0", 0, 0, &pixelShaderBlob, &errorMessages) != S_OK)
        {
            char const *errors = (char const *)errorMessages->GetBufferPointer();
            (void)errors;
            ASSERT ( false );
            return;
        }

        if (g_pd3dDevice->CreateVertexShader(vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), nullptr, &(pipeline->vertexShader)) != S_OK)
        {
            ASSERT ( false );
            return;
        }
        if (g_pd3dDevice->CreatePixelShader(pixelShaderBlob->GetBufferPointer(), pixelShaderBlob->GetBufferSize(), nullptr, &(pipeline->pixelShader)) != S_OK)
        {
            ASSERT ( false );
            return;
        }

        if ( pipelineNum == PipelineState::Pipeline_Wireframe )
        {
            if (g_pd3dDevice->CreateInputLayout(g_VertexInputWireframeDesc, ARRAYSIZE(g_VertexInputWireframeDesc), vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &(pipeline->inputLayout)) != S_OK)
            {
                return;
            }
        }
        else
        {
            if (g_pd3dDevice->CreateInputLayout(g_VertexInputDesc, ARRAYSIZE(g_VertexInputDesc), vertexShaderBlob->GetBufferPointer(), vertexShaderBlob->GetBufferSize(), &(pipeline->inputLayout)) != S_OK)
            {
                return;
            }
        }
    }
}

void CatchUpSimulationTime ( NtpTimeStamp renderTime )
{
    // Having a separate world time step might seem like overkill for a simple graphics demo,
    // but it's something I always forget to do until too late and then I have to disentangle everything.
    // So might as well start with one.

    // Keep simulating a fixed timestep until we have simulated past render time.
    // Then when we go to render, it will interpolate between the last two sim steps.
    while ( renderTime > g_TimeStampGameClockSim )
    {
        float deltaSeconds = g_SimulationTimeStepSeconds;
        NtpTimeStamp newSimTime = g_TimeStampGameClockSim.SecondsAfter ( g_SimulationTimeStepSeconds );

        for ( int objNum = 0; objNum < g_NumObjects; objNum++ )
        {
            GameObject *object = &(g_GameObjects[objNum]);

            // Get the current orientation, and start the sim.
            WorldOrientation const& worldOrn = object->worldFromObject.StartSimulationUpdate ( nullptr, newSimTime );

            float spinScale = deltaSeconds * 1.0f;
            float moveScale = deltaSeconds * 1.0f;

            Rot spinMatrix = Rot::MakeRotateX ( object->spin.x * spinScale );
            spinMatrix    *= Rot::MakeRotateY ( object->spin.y * spinScale );
            spinMatrix    *= Rot::MakeRotateZ ( object->spin.z * spinScale );
            Rot worldFromObject = worldOrn.worldFromObject;
            Dir worldMovement = worldFromObject * Dir ( 0.0f, 0.0f, object->speed * moveScale );
            WorldOrientation newOrn;
            newOrn.worldFromObject = worldFromObject * spinMatrix;
            newOrn.posWorld = worldOrn.posWorld.AddMeters ( worldMovement );
            // Note - in a real game it would be good to now and then reorthogonalise these,
            // or alternatively store the orientation as a quaternion instead.

            object->worldFromObject.FinishSimulationUpdate ( newSimTime, newOrn, false );
        }
        g_TimeStampGameClockSim = newSimTime;
    }
}


