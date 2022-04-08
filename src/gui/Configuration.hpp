#pragma once

#include "ConfigurationBackup.hpp"
#include <wx/wx.h>
#include <wx/aui/aui.h>
#include <wx/stc/stc.h>
#include <wx/treelist.h>
#include <wx/spinctrl.h>

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

class MacroRecordBoxDialog : public wxDialog
{
public:
	MacroRecordBoxDialog(wxWindow* parent);

	void ShowDialog(const wxString& str);
	uint8_t GetType() { return m_RecordType->GetSelection(); }
	bool IsApplyClicked() { return m_IsApplyClicked; }
protected:
	void OnApply(wxCommandEvent& event);
private:
	wxRadioBox* m_RecordType;
	wxStaticText* m_labelResult;
	bool m_IsApplyClicked;

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(MacroRecordBoxDialog);
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
	void OnTimer(wxTimerEvent& event);
private:
	wxRadioBox* m_radioBox1;
	wxTextCtrl* m_textMsg;
	wxStaticText* m_labelResult;
	bool m_IsApplyClicked;
	wxTimer* m_timer;

	wxDECLARE_EVENT_TABLE();
	wxDECLARE_NO_COPY_CLASS(MacroEditBoxDialog);
};

class ComTcpPanel : public wxPanel
{
public:
	ComTcpPanel(wxWindow* parent);
	void UpdatePanel();

	wxCheckBox* m_IsTcp;
	wxSpinCtrl* m_TcpPortSpin;
	wxCheckBox* m_IsPerAppMacro;
	wxCheckBox* m_IsAdvancedMacro;
	wxCheckBox* m_IsCom;
	wxComboBox* m_serial;
	wxTextCtrl* m_PathSepReplacerKey;
	wxCheckBox* m_IsMinimizeOnExit;
	wxCheckBox* m_IsMinimizeOnStartup;
	wxCheckBox* m_RememberWindowSize;
	wxCheckBox* m_AlwaysOnNumlock;
	wxSpinCtrl* m_DefaultPage;
	wxTextCtrl* m_ScreenshotKey;
	wxTextCtrl* m_ScreenshotDateFmt;
	wxTextCtrl* m_ScreenshotPath;
	wxTextCtrl* m_BackupDateFmt;
	wxCheckBox* m_IsSymlink;
	wxTextCtrl* m_MarkSymlink;
	wxTextCtrl* m_CreateSymlink;
	wxTextCtrl* m_CreateHardlink;
	wxCheckBox* m_IsAntiLock;
	wxCheckBox* m_IsScreensSaverAfterLock;
	wxSpinCtrl* m_AntiLockTimeout;

private:
	wxButton* m_Ok;
	wxButton* m_Backup;

	wxDECLARE_EVENT_TABLE();
};

class IKey;
class KeybrdPanel : public wxPanel
{
public:
	KeybrdPanel(wxWindow* parent);

	void OnTreeListChanged_Main(wxTreeListEvent& evt);
	void OnTreeListChanged_Details(wxTreeListEvent& evt);
	void OnItemContextMenu_Main(wxTreeListEvent& evt);
	void OnItemContextMenu_Details(wxTreeListEvent& evt);
	void OnItemActivated(wxTreeListEvent& event);
	void UpdateMainTree();
	void UpdateDetailsTree(std::unique_ptr<IKey>* ptr = nullptr);
	void OnKeyDown(wxKeyEvent& evt);

private:
	void ShowAddDialog();
	void ShowEditDialog(wxTreeListItem item);
	void DuplicateMacro(std::vector<std::unique_ptr<IKey>>& x, uint16_t id);
	void ManipulateMacro(std::vector<std::unique_ptr<IKey>>& x, uint16_t id, bool add);

	void TreeDetails_DeleteSelectedMacros();
	void TreeDetails_AddNewMacro();
	void TreeDetails_CloneMacro();
	void TreeDetails_MoveUpSelectedMacro();
	void TreeDetails_MoveDownSelectedMacro();
	void TreeDetails_StartRecording();

	std::vector<std::unique_ptr<IKey>>* GetKeyClassByItem(wxTreeListItem item, uint16_t& id);

	wxTreeListCtrl* tree;
	wxTreeListCtrl* tree_details;
	wxBitmapButton* btn_add;
	wxBitmapButton* btn_record;
	wxBitmapButton* btn_copy;
	wxBitmapButton* btn_delete;
	wxBitmapButton* btn_up;
	wxBitmapButton* btn_down;
	wxButton* m_Ok;
	MacroRecordBoxDialog* record_dlg;
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
	void UpdateSubpanels();

	wxAuiNotebook* m_notebook;
	ComTcpPanel* comtcp_panel;
	KeybrdPanel* keybrd_panel;
	BackupPanel* backup_panel;

private:
	void OnSize(wxSizeEvent& evt);
	wxDECLARE_EVENT_TABLE();
};