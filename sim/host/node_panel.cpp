// Standalone control panel. Runs ONE node (a controllable peer/contact) and
// joins the UDP multicast "ether", so it talks to a SEPARATE sim process (e.g.
// ./meshnode-companion-ui) over real mesh packets. Run them in two terminals:
//
//   terminal 1:  ./sim/meshnode-companion-ui     (the device, with its screen)
//   terminal 2:  ./sim/meshnode-panel            (this, drive the device)
//
// Commands:
//   advert            announce ourself (device adds us as a contact)
//   send <text>       send a DM to the device
//   contacts          show the device we've discovered
//   help / quit
#include "peer.h"
#include <cstdio>
#include <string>
#include <unistd.h>
#include <sys/select.h>

static Peer panel;

static void prompt() { fputs("> ", stdout); fflush(stdout); }

static void runCommand(const std::string& line) {
  if (line == "advert") panel.advertise();
  else if (line.rfind("send ", 0) == 0) panel.sendText(line.c_str() + 5);
  else if (line == "contacts") printf("[net] device contact: %s\n", panel.mesh.deviceName());
  else if (line == "help") printf("[net] commands: advert | send <text> | contacts | quit\n");
  else if (!line.empty()) printf("[net] unknown: '%s' (try 'help')\n", line.c_str());
}

int main() {
  printf("=== MeshCore control panel ===\n");
  panel.begin(0x9A11E1);                        // stable peer identity
  if (!panel.radio.enableUdpEther()) {
    printf("ERROR: could not join the UDP ether (multicast). Is it blocked?\n");
    return 1;
  }
  printf("joined ether 239.71.71.71:7337. Run ./sim/meshnode-companion-ui in another terminal.\n");
  printf("type 'help'. (start with 'advert', then 'send hello')\n");
  prompt();

  panel.advertise();   // announce once on startup (then only on the 'advert' command)

  std::string line;
  bool running = true;
  while (running) {
    panel.loop();

    fd_set fds; FD_ZERO(&fds); FD_SET(STDIN_FILENO, &fds);
    struct timeval tv{0, 5000};
    if (select(STDIN_FILENO + 1, &fds, nullptr, nullptr, &tv) > 0) {
      char ch; if (read(STDIN_FILENO, &ch, 1) == 1) {
        if (ch == '\n') {
          if (line == "quit" || line == "q") running = false;
          else { runCommand(line); prompt(); }
          line.clear();
        } else line += ch;
      }
    }
  }
  printf("\nbye.\n");
  return 0;
}
