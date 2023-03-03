#include "slframe.h"
#include "slcompressor.h"
#include "slinput.h"


bool App::OnInit() {
    wxFrame* window = new wxFrame(NULL, wxID_ANY, "GUI Test", wxDefaultPosition, wxSize(800, 700));

    wxPanel* panel = new wxPanel(window, wxID_ANY, wxDefaultPosition, wxSize(50, 100));
    wxButton* button = new wxButton(panel, wxID_ANY, "Click me!", wxPoint(20, 20), wxSize(100, 50));

    button->Bind(wxEVT_BUTTON, &App::onButtonClicked, this);

    window->Show();
    return true;
}

void App::onButtonClicked(wxCommandEvent& event) {
    SLinput input_;
    SLcompressor compress_;

    input_.open_media_input();
    input_.open_decoder_ctx();
    input_.open_encoder_ctx();
    input_.write_file_header();

    compress_.start_compress();
}

wxIMPLEMENT_APP(App);