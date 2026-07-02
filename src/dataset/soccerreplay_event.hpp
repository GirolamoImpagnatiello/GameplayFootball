#ifndef _HPP_SOCCERREPLAY_EVENT
#define _HPP_SOCCERREPLAY_EVENT

#include <string>

namespace dataset {
namespace soccerreplay1988 {

struct EventDescription {
  int half;
  std::string time_stamp;
  std::string comments_type;
  std::string comments_text;
  std::string comments_text_anonymized;
};

}
}

#endif
