#include "slframe.h"
#include <memory.h>


SLframe::SLframe() 
    : compressor_(std::make_shared<SLcompressor>("res/indigo.mp4")) {}

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
    compressor_thread_ = std::thread(&SLframe::compress, this);
    compressor_thread_.detach();
}

void SLframe::setup_ctx() {
    compressor_->open_media_input();
    compressor_->open_decoder_ctx();
    compressor_->open_encoder_ctx();
    compressor_->write_file_header();
}

void SLframe::compress() {
    setup_ctx();
    compressor_->start_compress();
    compressor_.reset(); // Release the compressor resources
}

wxIMPLEMENT_APP(SLframe);