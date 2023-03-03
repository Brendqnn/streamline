#include "slframe.h"
#include "slcompressor.h"


bool App::OnInit() {
    wxFrame* window = new wxFrame(NULL, wxID_ANY, "GUI Test", wxDefaultPosition, wxSize(800, 700));

    wxPanel* panel = new wxPanel(window, wxID_ANY, wxDefaultPosition, wxSize(50, 100));
    wxButton* button = new wxButton(panel, wxID_ANY, "Click me!", wxPoint(20, 20), wxSize(100, 50));

    button->Bind(wxEVT_BUTTON, &App::onButtonClicked, this);

    window->Show();
    return true;
}

void App::onButtonClicked(wxCommandEvent& event) {
    
    SLcompressor my_compressor("res/indigo.mp4");

    my_compressor.open_media_input();
    my_compressor.open_decoder_ctx();
    my_compressor.open_encoder_ctx();
    my_compressor.write_file_header();

    my_compressor.start_compress();
}

wxIMPLEMENT_APP(App);