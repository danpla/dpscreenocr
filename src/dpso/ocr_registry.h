
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
// * All DpsoOcr objects should be notified when ocr::LangManager is
//   about to be created so that they can finish recognition.
//
// * All DpsoOcr objects should also know when ocr::LangManager is
//   deleted so that they can refresh the cached language lists.
//
// * An DpsoOcr object should be able to check if ocr::LangManager is
//   active in order to reject queuing new OCR jobs and avoid the
//   case when both ocr::Recognizer and ocr::LangManager are accessed
//   concurrently from different threads.
//
// * All of the above should work regardless of whether DpsoOcr is
//   created before ocr::LangManager or vice versa.
class OcrRegistry {
public:
    static std::shared_ptr<OcrRegistry> get(
        const char* engineId, const char* dataDir);

    void add(DpsoOcr& ocr);
    void remove(DpsoOcr& ocr);

    void langManagerAboutToBeCreated();
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
