#pragma once
#include <wx/wx.h>
#include <memory>
#include <thread>
#include "slcompressor.h"

class SLframe : public wxApp {
public:
    SLframe();
    ~SLframe();

    bool OnInit();

    void onButtonClicked(wxCommandEvent& event);
private:
    std::shared_ptr<SLcompressor> compressor_;

    std::thread compressor_thread_;

    void compress();
    void setup_ctx();
};
