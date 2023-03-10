#pragma once
#include <wx/wx.h>
#include <memory>
#include <thread>
#include <sl_compressor.h>

class SLframe : public wxApp {
public:
    SLframe();
    ~SLframe();

    bool OnInit();

    void onButtonClicked(wxCommandEvent& event);
private:
    std::shared_ptr<SLcompressor> _compress = std::make_shared<SLcompressor>();
    void compress();
    
};
