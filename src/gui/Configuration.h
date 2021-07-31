#pragma once

#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <wx/stc/stc.h>
#include <wx/treelist.h>

template <class T>
class wxIntClientData : public wxClientData
{
public:
	wxIntClientData(T val) : value(val) {}
	~wxIntClientData() {}

	T GetValue() { return value; }
private:
	T value;
};

class MacroAddBoxDialog : public wxDialog
{
public:
	MacroAddBoxDialog(wxWindow* parent);

	void ShowDialog(const wxString& macro_key, const wxString& macro_name);
	wxString GetMacroName() { return m_macroName->GetValue(); }
	wxString GetMacroKey() { return m_macroKey->GetValue(); }
	bool IsApplyClicked() { return m_IsApplyClicked; }
protected:
	void OnApply(wxCommandEvent& event);
private:
	wxTextCtrl* m_macroName;
	wxTextCtrl* m_macroKey;
	wxStaticText* m_labelResult;
	bool m_IsApplyClicked;

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(MacroAddBoxDialog);
};

class MacroEditBoxDialog : public wxDialog
{
public:
	MacroEditBoxDialog(wxWindow* parent);

	void ShowDialog(const wxString& str, uint8_t macro_type);
	uint8_t GetType() { return m_radioBox1->GetSelection(); }
	wxString GetText() { return m_textMsg->GetValue(); }
	bool IsApplyClicked() { return m_IsApplyClicked; }
protected:
	void OnApply(wxCommandEvent& event);
private:
	wxRadioBox* m_radioBox1;
	wxTextCtrl* m_textMsg;
	wxStaticText* m_labelResult;
	bool m_IsApplyClicked;

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(MacroEditBoxDialog);
};

class ComTcpPanel : public wxPanel
{
public:
	ComTcpPanel(wxWindow* parent);
	wxCheckBox* m_IsTcp;
	wxSpinCtrl* m_TcpPortSpin;
	wxCheckBox* m_IsCom;
	wxComboBox* m_serial;
	wxCheckBox* m_IsMinimizeOnExit;
	wxCheckBox* m_IsMinimizeOnStartup;
	wxSpinCtrl* m_DefaultPage;

private:
	wxButton* m_Ok;

	wxDECLARE_EVENT_TABLE();
};

class KeyClass;
class KeybrdPanel : public wxPanel
{
public:
	KeybrdPanel(wxWindow* parent);

	void OnTreeListChanged_Main(wxTreeListEvent& evt);
	void OnTreeListChanged_Details(wxTreeListEvent& evt);
	void OnItemContextMenu_Main(wxTreeListEvent& event);
	void OnItemContextMenu_Details(wxTreeListEvent& event);
	void OnItemActivated(wxTreeListEvent& event);

private:
	void ShowAddDialog();
	void ShowEditDialog(wxTreeListItem item);
	void UpdateMainTree();
	void UpdateDetailsTree();
	void DuplicateMacro(std::vector<std::unique_ptr<KeyClass>>& x, uint16_t id);
	void ManipulateMacro(std::vector<std::unique_ptr<KeyClass>>& x, uint16_t id, bool add);

	wxTreeListCtrl* tree;
	wxTreeListCtrl* tree_details;
	wxBitmapButton* btn_add;
	wxBitmapButton* btn_copy;
	wxBitmapButton* btn_delete;
	wxBitmapButton* btn_up;
	wxBitmapButton* btn_down;
	MacroEditBoxDialog* edit_dlg;
	MacroAddBoxDialog* add_dlg;

	wxString root_sel_str;
	wxString child_sel_str;
	wxDECLARE_EVENT_TABLE();
};

class ConfigurationPanel : public wxPanel
{
public:
	ConfigurationPanel(wxWindow* parent);
	void Changeing(wxAuiNotebookEvent& event);

	ComTcpPanel* comtcp_panel;
	KeybrdPanel* keybrd_panel;

private:
	wxDECLARE_EVENT_TABLE();
};