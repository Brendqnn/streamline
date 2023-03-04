#include "slframe.h"
#include <memory.h>
#include "slcompressor.h"


bool App::OnInit() {
    wxFrame* window = new wxFrame(NULL, wxID_ANY, "Streamline v0.1", wxDefaultPosition, wxSize(800, 700));

    wxPanel* panel = new wxPanel(window, wxID_ANY, wxDefaultPosition, wxSize(30, 100));
    wxButton* button = new wxButton(panel, wxID_ANY, "Compress", wxPoint(20, 20), wxSize(100, 30));

    button->Bind(wxEVT_BUTTON, &App::onButtonClicked, this);

    window->Show();
    return true;
}

void App::onButtonClicked(wxCommandEvent& event) {
    std::shared_ptr<SLcompressor> my_compressor = std::make_shared<SLcompressor>("res/indigo.mp4");
    my_compressor->open_media_input();
    my_compressor->open_decoder_ctx();
    my_compressor->open_encoder_ctx();
    my_compressor->write_file_header();
    my_compressor->start_compress();
}

wxIMPLEMENT_APP(App);