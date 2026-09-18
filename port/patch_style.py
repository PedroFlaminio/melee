with open('src/render/settings_ui.cpp', 'r') as f:
    content = f.read()

style_code = """        ImGui::StyleColorsDark();
        
        ImGuiStyle& style = ImGui::GetStyle();
        ImVec4* colors = style.Colors;
        
        // Base Melee Violet: #4B237D (75, 35, 125) -> (0.29f, 0.14f, 0.49f)
        ImVec4 melee_violet        = ImVec4(0.294f, 0.137f, 0.490f, 1.000f);
        ImVec4 melee_violet_light  = ImVec4(0.420f, 0.239f, 0.616f, 1.000f);
        ImVec4 melee_violet_lighter= ImVec4(0.545f, 0.365f, 0.741f, 1.000f);
        ImVec4 melee_violet_dark   = ImVec4(0.169f, 0.075f, 0.302f, 1.000f);
        ImVec4 melee_violet_darker = ImVec4(0.100f, 0.040f, 0.180f, 1.000f);

        colors[ImGuiCol_WindowBg]           = ImVec4(0.06f, 0.06f, 0.06f, 0.94f);
        colors[ImGuiCol_TitleBg]            = melee_violet_dark;
        colors[ImGuiCol_TitleBgActive]      = melee_violet;
        colors[ImGuiCol_TitleBgCollapsed]   = melee_violet_darker;
        
        colors[ImGuiCol_Header]             = melee_violet;
        colors[ImGuiCol_HeaderHovered]      = melee_violet_light;
        colors[ImGuiCol_HeaderActive]       = melee_violet_lighter;
        
        colors[ImGuiCol_Button]             = melee_violet;
        colors[ImGuiCol_ButtonHovered]      = melee_violet_light;
        colors[ImGuiCol_ButtonActive]       = melee_violet_lighter;
        
        colors[ImGuiCol_FrameBg]            = melee_violet_dark;
        colors[ImGuiCol_FrameBgHovered]     = melee_violet;
        colors[ImGuiCol_FrameBgActive]      = melee_violet_light;
        
        colors[ImGuiCol_CheckMark]          = melee_violet_lighter;
        colors[ImGuiCol_SliderGrab]         = melee_violet_light;
        colors[ImGuiCol_SliderGrabActive]   = melee_violet_lighter;
        
        colors[ImGuiCol_SeparatorHovered]   = melee_violet_light;
        colors[ImGuiCol_SeparatorActive]    = melee_violet_lighter;
        colors[ImGuiCol_ResizeGrip]         = melee_violet;
        colors[ImGuiCol_ResizeGripHovered]  = melee_violet_light;
        colors[ImGuiCol_ResizeGripActive]   = melee_violet_lighter;
        
        colors[ImGuiCol_Tab]                = melee_violet_dark;
        colors[ImGuiCol_TabHovered]         = melee_violet_light;
        colors[ImGuiCol_TabActive]          = melee_violet;
        colors[ImGuiCol_TabUnfocused]       = melee_violet_darker;
        colors[ImGuiCol_TabUnfocusedActive] = melee_violet_dark;
"""

content = content.replace("        ImGui::StyleColorsDark();\n", style_code)

with open('src/render/settings_ui.cpp', 'w') as f:
    f.write(content)
