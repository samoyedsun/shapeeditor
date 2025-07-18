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
// 网格设置
const int GRID_SIZE = 15;      // 网格尺寸（奇数确保有中心点）
const int CELL_SIZE = 30;      // 每个格子像素大小
const ImVec4 SELECTED_COLOR = ImVec4(0.26f, 0.59f, 0.98f, 0.7f); // 选中格子颜色

// 网格状态
struct Cell {
    bool selected = false;
};
std::vector<std::vector<Cell>> grid(GRID_SIZE, std::vector<Cell>(GRID_SIZE));

// 偏移计算模式
enum OffsetMode {
    ORTHOGONAL,  // 正方向（垂直/水平）
    DIAGONAL     // 斜方向（45度对角线）
};
OffsetMode offsetMode = ORTHOGONAL;

// 计算中心坐标
ImVec2 gridCenter() {
    float centerX = ImGui::GetIO().DisplaySize.x * 0.5f;
    float centerY = ImGui::GetIO().DisplaySize.y * 0.5f;
    return ImVec2(centerX, centerY);
}

// 绘制网格
void DrawGrid() {
    ImDrawList* draw_list = ImGui::GetBackgroundDrawList();
    ImVec2 center = gridCenter();
    ImVec2 gridStart(center.x - (GRID_SIZE/2)*CELL_SIZE, 
                     center.y - (GRID_SIZE/2)*CELL_SIZE);

    // 绘制格子和选择状态
    for (int y = 0; y < GRID_SIZE; ++y) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            ImVec2 rect_min(gridStart.x + x*CELL_SIZE, gridStart.y + y*CELL_SIZE);
            ImVec2 rect_max(rect_min.x + CELL_SIZE, rect_min.y + CELL_SIZE);
            
            // 绘制选中格子
            if (grid[y][x].selected) {
                draw_list->AddRectFilled(rect_min, rect_max, 
                    ImColor(SELECTED_COLOR), 0.0f);
            }
            
            // 绘制格子边框
            draw_list->AddRect(rect_min, rect_max, IM_COL32(200, 200, 200, 255), 
                               0.0f, 0, 1.0f);
        }
    }
    
    // 绘制中心标记
    const float cross_size = 8.0f;
    draw_list->AddLine(
        ImVec2(center.x - cross_size, center.y),
        ImVec2(center.x + cross_size, center.y),
        IM_COL32(255, 0, 0, 255), 2.0f);
    draw_list->AddLine(
        ImVec2(center.x, center.y - cross_size),
        ImVec2(center.x, center.y + cross_size),
        IM_COL32(255, 0, 0, 255), 2.0f);
    
    // 绘制参考方向指示器
    const float arrow_size = 20.0f;
    if (offsetMode == ORTHOGONAL) {
        // 正交方向箭头
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
        // 斜方向箭头
        draw_list->AddLine(center, 
                          ImVec2(center.x + arrow_size * 1.414f, center.y + arrow_size * 1.414f), 
                          IM_COL32(0, 255, 255, 255), 2.0f);
        draw_list->AddLine(center, 
                          ImVec2(center.x - arrow_size * 1.414f, center.y + arrow_size * 1.414f), 
                          IM_COL32(0, 255, 255, 255), 2.0f);
        
        // 箭头头部
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

// 处理鼠标选择
void HandleSelection() {
    if (!ImGui::IsMouseClicked(0)) return;
    
    ImVec2 mouse_pos = ImGui::GetMousePos();
    ImVec2 center = gridCenter();
    ImVec2 gridStart(center.x - (GRID_SIZE/2)*CELL_SIZE, 
                     center.y - (GRID_SIZE/2)*CELL_SIZE);
    
    // 计算点击的网格坐标
    int gridX = static_cast<int>((mouse_pos.x - gridStart.x) / CELL_SIZE);
    int gridY = static_cast<int>((mouse_pos.y - gridStart.y) / CELL_SIZE);
    
    // 检查是否在网格范围内
    if (gridX >= 0 && gridX < GRID_SIZE && gridY >= 0 && gridY < GRID_SIZE) {
        grid[gridY][gridX].selected = !grid[gridY][gridX].selected; // 切换选中状态
    }
}

// 导出到CSV
void ExportToCSV(const std::string& filename) {
    std::ofstream file(filename);
    if (!file.is_open()) return;
    
    // 写入CSV标题
    file << "Grid X,Grid Y,Ortho Offset X,Ortho Offset Y,Diag Offset U,Diag Offset V\n";
    
    // 中心坐标（网格坐标系）
    int centerX = GRID_SIZE / 2;
    int centerY = GRID_SIZE / 2;
    
    // 遍历所有格子
    for (int y = 0; y < GRID_SIZE; ++y) {
        for (int x = 0; x < GRID_SIZE; ++x) {
            if (grid[y][x].selected) {
                // 计算正交偏移（正方向）
                int offsetX = x - centerX;
                int offsetY = y - centerY;
                
                // 计算斜方向偏移（45度对角线方向）
                // 使用轴对齐的等距坐标：U = X - Y, V = X + Y
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
    ImGui::Begin(WcharToChar(std::wstring(L"控制面板")).c_str());
    
    // 显示状态信息
    ImGui::Text(WcharToChar(std::wstring(L"当前选择: \n%d个格子")).c_str(), []{
        int count = 0;
        for (const auto& row : grid) for (const auto& cell : row)
            if (cell.selected) count++;
        return count;
    }());
    
    // 偏移模式选择
    ImGui::Separator();
    ImGui::Text(WcharToChar(std::wstring(L"参考方向:")).c_str());
    ImGui::RadioButton(WcharToChar(std::wstring(L"正方向 (垂直/水平)")).c_str(), (int*)&offsetMode, ORTHOGONAL);
    //ImGui::SameLine();
    ImGui::RadioButton(WcharToChar(std::wstring(L"斜方向 (45度对角线)")).c_str(), (int*)&offsetMode, DIAGONAL);
    
    // 导出按钮
    ImGui::Separator();
    if (ImGui::Button(WcharToChar(std::wstring(L"导出CSV")).c_str())) {
        ExportToCSV("grid_offsets.csv");
        ImGui::OpenPopup(WcharToChar(std::wstring(L"导出成功")).c_str());
    }
    // 清空按钮
    //ImGui::SameLine();
    if (ImGui::Button(WcharToChar(std::wstring(L"清空选择")).c_str())) {
        for (auto& row : grid) for (auto& cell : row)
            cell.selected = false;
    }
    
    // 导出成功提示
    if (ImGui::BeginPopupModal(WcharToChar(std::wstring(L"导出成功")).c_str(), NULL, ImGuiWindowFlags_AlwaysAutoResize)) {
        ImGui::Text(WcharToChar(std::wstring(L"已导出到 grid_offsets.csv")).c_str());
        ImGui::Text(WcharToChar(std::wstring(L"包含正方向和斜方向偏移量")).c_str());
        if (ImGui::Button(WcharToChar(std::wstring(L"确定")).c_str())) ImGui::CloseCurrentPopup();
        ImGui::EndPopup();
    }
    
    // 帮助文本
    ImGui::Separator();
    ImGui::TextWrapped(WcharToChar(std::wstring(L"操作说明: 点击格子选择/取消选择，选择参考方向后导出CSV")).c_str());
    
    ImGui::End();
}

int main(int argc, char* argv[])
{
    std::cout << "Tool Launch Success!" << std::endl;
    // 初始化 GLFW
    if (!glfwInit())
    {
        return -1;
    }
    ShowWindow(GetConsoleWindow(), SW_HIDE);
    //SetConsoleOutputCP(CP_UTF8);
    // 创建窗口和上下文
    GLFWwindow* window = glfwCreateWindow(1000, 800, "shapeeditor create by ImGui", NULL, NULL);
    if (!window)
    {
        glfwTerminate();
        return -1;
    }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    // 初始化 OpenGL3
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        return -1;
    }
    // 初始化 Dear ImGui
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 130");

    ImGuiIO& io = ImGui::GetIO();
    ImFont* font = io.Fonts->AddFontFromFileTTF("C:\\Windows\\Fonts\\msyh.ttc", 28.0f, NULL, io.Fonts->GetGlyphRangesChineseSimplifiedCommon());
    IM_ASSERT(font != NULL);
    ImGuiStyle* style = &ImGui::GetStyle();
    style->Colors[ImGuiCol_Text] = ImVec4(0.0f, 1.0f, 0.0f, 1.0f);

    // 主循环
    while (!glfwWindowShouldClose(window))
    {
        glfwPollEvents();

        // 开始 Dear ImGui 帧
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();
        
        // 处理鼠标选择
        HandleSelection();
        // 创建控制面板
        DrawConvertWindow();
        // 绘制网格（在背景上）
        DrawGrid();

        // 渲染 Dear ImGui
        ImGui::Render();
        int display_w, display_h;
        glfwGetFramebufferSize(window, &display_w, &display_h);
        glViewport(0, 0, display_w, display_h);
        glClearColor(0.2f, 0.0f, 0.2f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        glfwSwapBuffers(window);
    }

    // 清理和销毁 Dear ImGui 上下文
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();

    // 清理 GLFW 和 OpenGL3
    glfwDestroyWindow(window);
    glfwTerminate();

    return 0;
}
