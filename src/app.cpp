#include "app.h"

namespace xpilot
{
    App::App(QObject *owner) : QObject(owner)
    {
        connect(&fsd, &FsdClient::RaiseServerIdentificationReceived, this, &App::HandleServerIdentification);
        fsd.Connect("192.168.50.56", 6809);
    }

    void App::HandleServerIdentification(PDUServerIdentification pdu)
    {
        fsd.SendPDU(PDUClientIdentification("FLC1", 0xd8f2, "xPilot", 1, 0, "", "", ""));

        fsd.SendPDU(PDUAddPilot("FLC1", "1215759", "3rBaF46EQ3tRNWe", NetworkRating::OBS, ProtocolRevision::VatsimAuth, SimulatorType::XPlane, "Justin Shannon"));
    }
}
