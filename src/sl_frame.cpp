#include "sl_frame.h"
#include <memory.h>


SLframe::SLframe() {}
    

SLframe::~SLframe() {}

bool SLframe::OnInit() {
    wxFrame* window = new wxFrame(NULL, wxID_ANY, "Streamline v0.1", wxDefaultPosition, wxSize(800, 700));
    wxPanel* panel = new wxPanel(window, wxID_ANY, wxDefaultPosition, wxSize(30, 100));
    wxButton* button = new wxButton(panel, wxID_ANY, "Compress", wxPoint(20, 20), wxSize(100, 30));


    button->Bind(wxEVT_BUTTON, &SLframe::onButtonClicked, this);
    window->Show();


    return true;
}

void SLframe::onButtonClicked(wxCommandEvent& event) {
    _compress->setup_ctx();
    _compress->start_compress();
}


void SLframe::compress() {
    
}

wxIMPLEMENT_APP(SLframe);