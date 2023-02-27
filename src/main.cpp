#include <wx/wx.h>

class App : public wxApp {
public:
    bool OnInit() {
        wxFrame* window = new wxFrame(NULL, wxID_ANY, "GUI Test", wxDefaultPosition, wxSize(600, 400));

        wxPanel* panel = new wxPanel(window, wxID_ANY, wxDefaultPosition, wxSize(50, 100));
        wxButton* button = new wxButton(panel, wxID_ANY, "Click me!", wxPoint(20, 20), wxSize(150, 150));

        window->Show();
        return true;
    }
};

wxIMPLEMENT_APP(App);
