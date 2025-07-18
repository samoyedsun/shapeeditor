#include <iostream>
#include <fstream>
#include <vector>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include <GLFW/glfw3.h>
#include <glad/glad.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3native.h>

std::wstring CharToWchar(const std::string& mbstr) {
    const char* mbcstr = mbstr.c_str();
    int len = MultiByteToWideChar(CP_UTF8, 0, mbcstr, -1, nullptr, 0);
    if (len == 0) {
        std::cerr << "MultiByteToWideChar failed" << std::endl;
        return L"";
    }

    wchar_t* wcstr = new wchar_t[len];
    MultiByteToWideChar(CP_UTF8, 0, mbcstr, -1, wcstr, len);
    std::wstring result(wcstr);
    delete[] wcstr;
    return result;
}

std::string WcharToChar(const std::wstring& wstr) {
    const wchar_t* wccstr = wstr.c_str();
    int len = WideCharToMultiByte(CP_UTF8, 0, wccstr, -1, nullptr, 0, nullptr, nullptr);
    if (len == 0) {
        std::cerr << "WideCharToMultiByte failed" << std::endl;
        return "";
    }

    std::vector<char> mbstr(len);
    WideCharToMultiByte(CP_UTF8, 0, wccstr, -1, &mbstr[0], len, nullptr, nullptr);
    return std::string(mbstr.begin(), mbstr.end());
}

//=========================================================
// ��������
const int GRID_SIZE = 15;      // ����ߴ磨����ȷ�������ĵ㣩
const int CELL_SIZE = 30;      // ÿ���������ش�С
const ImVec4 SELECTED_COLOR = ImVec4(0.26f, 0.59f, 0.98f, 0.7f); // ѡ�и�����ɫ

// ����״̬
struct Cell {
    bool selected = false;
};
std::vector<std::vector<Cell>> grid(GRID_SIZE, std::vector<Cell>(GRID_SIZE));

// ƫ�Ƽ���ģʽ
enum OffsetMode {
    ORTHOGONAL,  // �����򣨴�ֱ/ˮƽ��
    DIAGONAL     // б����45�ȶԽ��ߣ�
};
OffsetMode offsetMode = ORTHOGONAL;

// ������������
ImVec2 gridCenter() {
    float centerX = ImGui::GetIO().DisplaySize.x * 0.5f;
    float centerY = ImGui::GetIO().DisplaySize.y * 0.5f;
    return ImVec2(centerX, centerY);
}

// ��������
void DrawGrid() {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    ImVec2 center = gridCenter();
    ImVec2 gridStart(center.x - (GRID_SIZE/2)*CELL_SIZE, 
                     center.y - (GRID_SIZE/2)*CELL_SIZE);

    // ���Ƹ��Ӻ�ѡ��״̬
    for (int y = 0; y < GRID_SIZE; ++y) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            ImVec2 rect_min(gridStart.x + x*CELL_SIZE, gridStart.y + y*CELL_SIZE);
            ImVec2 rect_max(rect_min.x + CELL_SIZE, rect_min.y + CELL_SIZE);
            
            // ����ѡ�и���
            if (grid[y][x].selected) {
                draw_list->AddRectFilled(rect_min, rect_max, 
                    ImColor(SELECTED_COLOR), 0.0f);
            }
            
            // ���Ƹ��ӱ߿�
            draw_list->AddRect(rect_min, rect_max, IM_COL32(200, 200, 200, 255), 
                               0.0f, 0, 1.0f);
        }
    }
    
    // �������ı��
    const float cross_size = 8.0f;
    draw_list->AddLine(
        ImVec2(center.x - cross_size, center.y),
        ImVec2(center.x + cross_size, center.y),
        IM_COL32(255, 0, 0, 255), 2.0f);
    draw_list->AddLine(
        ImVec2(center.x, center.y - cross_size),
        ImVec2(center.x, center.y + cross_size),
        IM_COL32(255, 0, 0, 255), 2.0f);
    
    // ���Ʋο�����ָʾ��
    const float arrow_size = 20.0f;
    if (offsetMode == ORTHOGONAL) {
        // ���������ͷ
        draw_list->AddLine(center, ImVec2(center.x + arrow_size * 2, center.y), 
                          IM_COL32(0, 255, 0, 255), 2.0f);
        draw_list->AddLine(center, ImVec2(center.x, center.y + arrow_size * 2), 
                          IM_COL32(0, 255, 0, 255), 2.0f);
        draw_list->AddTriangleFilled(
            ImVec2(center.x + arrow_size * 2, center.y - 3),
            ImVec2(center.x + arrow_size * 2, center.y + 3),
            ImVec2(center.x + arrow_size * 2 + 8, center.y),
            IM_COL32(0, 255, 0, 255));
        draw_list->AddTriangleFilled(
            ImVec2(center.x - 3, center.y + arrow_size * 2),
            ImVec2(center.x + 3, center.y + arrow_size * 2),
            ImVec2(center.x, center.y + arrow_size * 2 + 8),
            IM_COL32(0, 255, 0, 255));
    } else {
        // б�����ͷ
        draw_list->AddLine(center, 
                          ImVec2(center.x + arrow_size * 1.414f, center.y + arrow_size * 1.414f), 
                          IM_COL32(0, 255, 255, 255), 2.0f);
        draw_list->AddLine(center, 
                          ImVec2(center.x - arrow_size * 1.414f, center.y + arrow_size * 1.414f), 
                          IM_COL32(0, 255, 255, 255), 2.0f);
        
        // ��ͷͷ��
        draw_list->AddTriangleFilled(
            ImVec2(center.x + arrow_size * 1.414f - 5, center.y + arrow_size * 1.414f - 5),
            ImVec2(center.x + arrow_size * 1.414f + 5, center.y + arrow_size * 1.414f - 5),
            ImVec2(center.x + arrow_size * 1.414f, center.y + arrow_size * 1.414f + 5),
            IM_COL32(0, 255, 255, 255));
        draw_list->AddTriangleFilled(
            ImVec2(center.x - arrow_size * 1.414f - 5, center.y + arrow_size * 1.414f - 5),
            ImVec2(center.x - arrow_size * 1.414f + 5, center.y + arrow_size * 1.414f - 5),
            ImVec2(center.x - arrow_size * 1.414f, center.y + arrow_size * 1.414f + 5),
            IM_COL32(0, 255, 255, 255));
    }
}

// �������ѡ��
void HandleSelection() {
    if (!ImGui::IsMouseClicked(0)) return;
    
    ImVec2 mouse_pos = ImGui::GetMousePos();
    ImVec2 center = gridCenter();
    ImVec2 gridStart(center.x - (GRID_SIZE/2)*CELL_SIZE, 
                     center.y - (GRID_SIZE/2)*CELL_SIZE);
    
    // ����������������
    int gridX = static_cast<int>((mouse_pos.x - gridStart.x) / CELL_SIZE);
    int gridY = static_cast<int>((mouse_pos.y - gridStart.y) / CELL_SIZE);
    
    // ����Ƿ�������Χ��
    if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE) {
        grid[gridY][gridX].selected = !grid[gridY][gridX].selected; // �л�ѡ��״̬
    }
}

// ������CSV
void ExportToCSV(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    // д��CSV����
    file << "Grid X,Grid Y,Ortho Offset X,Ortho Offset Y,Diag Offset U,Diag Offset V\n";
    
    // �������꣨��������ϵ��
    int centerX = GRID_SIZE / 2;
    int centerY = GRID_SIZE / 2;
    
    // �������и���
    for (int y = 0; y < GRID_SIZE; ++y) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            if (grid[y][x].selected) {
                // ��������ƫ�ƣ�������
                int offsetX = x - centerX;
                int offsetY = y - centerY;
                
                // ����б����ƫ�ƣ�45�ȶԽ��߷���
                // ʹ�������ĵȾ����꣺U = X - Y, V = X + Y
                int diagU = (x - centerX) - (y - centerY);
                int diagV = (x - centerX) + (y - centerY);
                
                file << x << "," << y << "," 
                     << offsetX << "," << offsetY << ","
                     << diagU << "," << diagV << "\n";
            }
        }
    }
    file.close();
}

void DrawConvertWindow()
{
    ImGui::SetNextWindowPos(ImVec2(20, 20), ImGuiCond_FirstUseEver);
    ImGui::SetNextWindowSize(ImVec2(150, 500), ImGuiCond_FirstUseEver);
    ImGui::Begin(WcharToChar(std::wstring(L"�������")).c_str());
    
    // ��ʾ״̬��Ϣ
    ImGui::Text(WcharToChar(std::wstring(L"��ǰѡ��: \n%d������")).c_str(), []{
        int count = 0;
        for (const auto& row : grid) for (const auto& cell : row)
            if (cell.selected) count++;
        return count;
    }());
    
    // ƫ��ģʽѡ��
    ImGui::Separator();
    ImGui::Text(WcharToChar(std::wstring(L"�ο�����:")).c_str());
    ImGui::RadioButton(WcharToChar(std::wstring(L"������ (��ֱ/ˮƽ)")).c_str(), (int*)&offsetMode, ORTHOGONAL);
    //ImGui::SameLine();
    ImGui::RadioButton(WcharToChar(std::wstring(L"б���� (45�ȶԽ���)")).c_str(), (int*)&offsetMode, DIAGONAL);
    
    // ������ť
    ImGui::Separator();
    if (ImGui::Button(WcharToChar(std::wstring(L"����CSV")).c_str())) {
        ExportToCSV("grid_offsets.csv");
        ImGui::OpenPopup(WcharToChar(std::wstring(L"�����ɹ�")).c_str());
    }
    // ��հ�ť
    //ImGui::SameLine();
    if (ImGui::Button(WcharToChar(std::wstring(L"���ѡ��")).c_str())) {
        for (auto& row : grid) for (auto& cell : row)
            cell.selected = false;
    }
    
    // �����ɹ���ʾ
    if (ImGui::BeginPopupModal(WcharToChar(std::wstring(L"�����ɹ�")).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(WcharToChar(std::wstring(L"�ѵ����� grid_offsets.csv")).c_str());
        ImGui::Text(WcharToChar(std::wstring(L"�����������б����ƫ����")).c_str());
        if (ImGui::Button(WcharToChar(std::wstring(L"ȷ��")).c_str())) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    
    // �����ı�
    ImGui::Separator();
    ImGui::TextWrapped(WcharToChar(std::wstring(L"����˵��: �������ѡ��/ȡ��ѡ��ѡ��ο�����󵼳�CSV")).c_str());
    
    ImGui::End();
}

int main(int argc, char* argv[])
{
    std::cout << "Tool Launch Success!" << std::endl;
    // ��ʼ�� GLFW
    if (!glfwInit())
    {
        return -1;
    }
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    //SetConsoleOutputCP(CP_UTF8);
    // �������ں�������
    GLFWwindow* window = glfwCreateWindow(1000, 800, "shapeeditor create by ImGui", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    // ��ʼ�� OpenGL3
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        return -1;
    }
    // ��ʼ�� Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 28.0f, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    IM_ASSERT(font != NULL);
    ImGuiStyle* style = &ImGui::GetStyle();
    style->Colors[ImGuiCol_Text] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

    // ��ѭ��
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // ��ʼ Dear ImGui ֡
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // �������ѡ��
        HandleSelection();
        // �����������
        DrawConvertWindow();
        // ���������ڱ����ϣ�
        DrawGrid();

        // ��Ⱦ Dear ImGui
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.2f, 0.0f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // ��������� Dear ImGui ������
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // ���� GLFW �� OpenGL3
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
