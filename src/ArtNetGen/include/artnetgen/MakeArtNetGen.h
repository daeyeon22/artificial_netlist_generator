#ifndef MAKE_ART_NET_GEN
#define MAKE_ART_NET_GEN

namespace artnetgen {
class ArtNetGen ;
}

namespace ord {

class ArtNetGen;

artnetgen::ArtNetGen*
makeArtNetGen();

void
initArtNetGen(OpenRoad *openroad);

void
deleteArtNetGen(artnetgen::ArtNetGen *graphext);

}

#endif
