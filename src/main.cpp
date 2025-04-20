#include <drogon/drogon.h>

int main() {
    auto &app = drogon::app();
    app.addListener("0.0.0.0", 443);
    app.loadConfigFile("/app/config/config.yaml");
    app.run();
    return 0;
}
