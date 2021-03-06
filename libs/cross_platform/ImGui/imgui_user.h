#pragma once

#include <string>
#include <vector>

namespace ImGui
{
	inline bool InputText(const char* label, std::string& str, ImGuiInputTextFlags flags = 0, ImGuiInputTextCallback cb = (ImGuiInputTextCallback)0, void* user_data = nullptr)
	{
		char buf[1024];
		snprintf(buf, 1024, "%s", str.c_str());
		bool result = ImGui::InputText(label, buf, 1024, flags, cb, user_data);
		str = buf;
		return result;
	}

	namespace detail
	{
		inline bool item_getter_vec_string(void* data, int id, const char** out_str)
		{
			auto& vec = (*(std::vector<std::string>*)data);

			if (id >= 0 && id < (int)vec.size())
			{
				*out_str = vec[id].c_str();
				return true;
			}
			return false;
		}
	}

	inline bool Combo(const char* label, int* current_item, std::vector<std::string>& items, int height_in_items=-1)
	{
		return Combo(label, current_item, &detail::item_getter_vec_string, &items, (int)items.size(), height_in_items);
	}

	bool TreeNodeAdv(const char* label, ImGuiTreeNodeFlags flags);
	bool TreeNodeAdv(void* id, const char* label, ImGuiTreeNodeFlags flags);
}



struct lua_State;


namespace ImGui
{

IMGUI_API void ResetActiveID();
IMGUI_API int PlotHistogramEx(const char* label,
	float(*values_getter)(void* data, int idx),
	void* data,
	int values_count,
	int values_offset,
	const char* overlay_text,
	float scale_min,
	float scale_max,
	ImVec2 graph_size,
	int selected_index);

IMGUI_API bool ListBox(const char* label,
	int* current_item,
	int scroll_to_item,
	bool(*items_getter)(void*, int, const char**),
	void* data,
	int items_count,
	int height_in_items);
IMGUI_API bool ColorPicker(float* col, bool alphabar);

IMGUI_API void BringToFront();

IMGUI_API bool BeginToolbar(const char* str_id, ImVec2 screen_pos, ImVec2 size);
IMGUI_API void EndToolbar();
IMGUI_API bool ToolbarButton(ImTextureID texture, const ImVec4& bg_color, const char* tooltip);

IMGUI_API void BeginNode(ImGuiID id, ImVec2 screen_pos);
IMGUI_API void EndNode(ImVec2& pos);
IMGUI_API bool PinNode(ImGuiID id, ImVec2 screen_pos);
IMGUI_API void LinkNodes(ImVec2 from, ImVec2 to);
IMGUI_API ImVec2 GetNodeInputPos(ImGuiID node_id, int input);
IMGUI_API ImVec2 GetNodeOutputPos(ImGuiID node_id, int output);
IMGUI_API void NodeSlots(int count, bool input);

IMGUI_API bool BeginTimeline(const char* str_id, float max_value);
IMGUI_API bool TimelineEvent(const char* str_id, float* values);
IMGUI_API void EndTimeline();

struct CurveEditor
{
	bool valid;
	ImVec2 beg_pos;
	ImVec2 graph_size;
	static const float GRAPH_MARGIN;
	static const float HEIGHT;
	ImVec2 inner_bb_min;
	ImVec2 inner_bb_max;
	int point_idx;
};

IMGUI_API CurveEditor BeginCurveEditor(const char* label);
IMGUI_API bool CurveSegment(ImVec2* point, CurveEditor& editor);
IMGUI_API void EndCurveEditor(const CurveEditor& editor);
IMGUI_API bool BeginResizablePopup(const char* str_id, const ImVec2& size_on_first_use);
IMGUI_API void IntervalGraph(const unsigned long long* value_pairs,
	int value_pairs_count,
	unsigned long long scale_min,
	unsigned long long scele_max);
IMGUI_API bool FilterInput(const char* label, char* buf, size_t buf_size);
IMGUI_API void HSplitter(const char* str_id, ImVec2* size);
IMGUI_API void Rect(float w, float h, ImU32 color);

} // namespace ImGui



