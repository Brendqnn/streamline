#pragma once
#include <wx/wx.h>

class App : public wxApp {
public:
	bool OnInit();
	void onButtonClicked(wxCommandEvent& event);
};