
#pragma once

#include <memory>
#include <string>
#include <vector>


struct DpsoOcr;


namespace dpso::ocr {


// The class is a kind of poor man's observer to synchronize DpsoOcr
// objects and a language manager created for the same engine and data
// directory. It solves the following problems:
//
// * All DpsoOcr objects should be notified when the data manager is
//   created so that they can finish recognition. This is done via
//   langManagerCreated().
//
// * All DpsoOcr objects should also know when the data manager is
//   deleted so that they can refresh the cached language lists. This
//   is done via langManagerDeleted().
//
// * An DpsoOcr object should be able to check if the data manager is
//   active in order to reject queuing new OCR jobs. This is done via
//   getLangManagerIsActive().
//
// * All of the above should work regardless of whether DpsoOcr is
//   created before the language manager or vice versa.
class OcrRegistry {
public:
    static std::shared_ptr<OcrRegistry> get(
        const char* engineId, const char* dataDir);

    void add(DpsoOcr& ocr);
    void remove(DpsoOcr& ocr);

    void langManagerCreated();
    void langManagerDeleted();

    bool getLangManagerIsActive() const;
private:
    OcrRegistry(const char* engineId, const char* dataDir);

    std::string engineId;
    std::string dataDir;
    std::vector<DpsoOcr*> ocrs;
    bool langManagerIsActive;
};


}
