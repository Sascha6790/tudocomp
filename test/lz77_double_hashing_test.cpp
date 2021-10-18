#include <tudocomp/compressors/lz77/LZ77DoubleHashing.hpp>
#include <tudocomp/compressors/lzss/StreamingCoder.hpp>
#include "test/util.hpp"
#include <cstring>

using namespace tdc;
using coder = lzss::DidacticalCoder;

template<class C>
void decompress(const std::string compressedTest, std::string originalText, std::string options) {
    std::vector<uint8_t> decoded_buffer;
    {
        Input text_in = Input::from_memory(compressedTest);
        Output decoded_out = Output::from_memory(decoded_buffer);

        auto m_registry = RegistryOf<Compressor>();
        auto decompressor = m_registry.select<C>(options)->decompressor();
        decompressor->decompress(text_in, decoded_out);
    }
    std::string decompressed_text{
            decoded_buffer.begin(),
            decoded_buffer.end(),
    };
    ASSERT_EQ(originalText, decompressed_text);
}


TEST(LZ77DoubleHashing, Bit10) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    auto result = test::compress<lz77::LZ77DoubleHashing<lzss::DidacticalCoder>>(input, "HASH_BITS=10");
    ASSERT_EQ(result.str, "cbabc{3, 12}{16, 7}{8, 8}{3, 9}{16, 7}");
}

TEST(LZ77DoubleHashing, Bit8) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    auto result = test::compress<lz77::LZ77DoubleHashing<lzss::DidacticalCoder>>(input, "HASH_BITS=8");
    ASSERT_EQ(result.str, "cbabc{3, 12}{16, 7}{8, 8}{3, 9}{16, 7}");
}

TEST(LZ77DoubleHashing, Bit4) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    auto result = test::compress<lz77::LZ77DoubleHashing<lzss::DidacticalCoder>>(input, "HASH_BITS=4");
    ASSERT_EQ(result.str, "cbabc{3, 5}{3, 5}b{16, 5}{16, 3}cbabc{3, 5}{3, 5}b{16, 5}{16, 3}");
}

TEST(LZ77DoubleHashing, Bit4_Mode2) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    auto result = test::compress<lz77::LZ77DoubleHashing<lz77::LZ77StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(input, "HASH_BITS=4,COMPRESSION_MODE=2");
    ASSERT_EQ(result.str, "cbabc{3, 5}{3, 5}bc{16, 5}bccbabc{3, 5}{3, 5}bcbabc{3, 3}");
}

TEST(LZ77DoubleHashing, MODE_1) {
    const std::string input = "In Western astrology, astrological signs are the twelve 30° sectors of the ecliptic, starting at the vernal equinox (one of the intersections of the ecliptic with the celestial equator), also known as the First Point of Aries. The order of the astrological signs is Aries, Taurus, Gemini, Cancer, Leo, Virgo, Libra, Scorpio, Sagittarius, Capricorn, Aquarius and Pisces.\n"
                              "The concept of the zodiac originated in Babylonian astrology, and was later influenced by Hellenistic culture. According to astrology, celestial phenomena relate to human activity on the principle of \"as above, so below\", so that the signs are held to represent characteristic modes of expression.\n"
                              "The twelve sector division of the ecliptic constitutes astrology's primary frame of reference when considering the positions of celestial bodies, from a geocentric point of view, so that we may find, for instance, the Sun in 23° \"Aries\" (23° longitude), the Moon in 7° \"Scorpio\" (217° longitude), or Jupiter in 29° \"Pisces\" (359° longitude). Beyond the celestial bodies, other astrological points that are dependent on geographical location and time (namely, the Ascendant, the Midheaven, the Vertex and the houses' cusps) are also referenced within this ecliptic coordinate system.\n"
                              "Various approaches to measuring and dividing the sky are currently used by differing systems of astrology, although the tradition of the Zodiac's names and symbols remain consistent. Western astrology measures from Equinox and Solstice points (points relating to equal, longest and shortest days of the tropical year), while Jyotiṣa or Vedic astrology measures along the equatorial plane (sidereal year). Precession results in Western astrology's zodiacal divisions not corresponding in the current era to the constellations that carry similar names, while Jyotiṣa measurements still correspond with the background constellations.\n"
                              "In Western and Asian astrology, the emphasis is on space, and the movement of the Sun, Moon and planets in the sky through each of the zodiac signs. In Chinese astrology, by contrast, the emphasis is on time, with the zodiac operating on cycles of years, months, and hours of the day.\n"
                              "A common feature of all three traditions however, is the significance of the Ascendant — the zodiac sign that is rising (due to the rotation of the earth) on the eastern horizon at the moment of a person's birth.\n"
                              "While Western astrology is essentially a product of Greco-Roman culture, some of its more basic concepts originated in Babylonia. Isolated references to celestial \"signs\" in Sumerian sources are insufficient to speak of a Sumerian zodiac. Specifically, the division of the ecliptic in twelve equal sectors is a Babylonian conceptual construction.\n"
                              "By the 4th century BC, Babylonians' astronomy and their system of celestial omens were influencing the Greek culture and, by the late 2nd century BC, Egyptian astrology was also mixing in. This resulted, unlike the Mesopotamian tradition, in a strong focus on the birth chart of the individual and in the creation of horoscopic astrology, employing the use of the Ascendant (the rising degree of the ecliptic, at the time of birth), and of the twelve houses. Association of the astrological signs with Empedocles' four classical elements was another important development in the characterization of the twelve signs.\n"
                              "The body of astrological knowledge by the 2nd century AD is described in Ptolemy's \"Tetrabiblos\", a work that was responsible for astrology's successful spread across Europe and the Middle East, and remained a reference for almost seventeen centuries as later traditions made few substantial changes to its core teachings.\n"
                              "The following table enumerates the twelve divisions of celestial longitude, with the Latin names (still widely used) and the English translation (gloss). The longitude intervals, being a mathematical division, are closed for the first endpoint (\"a\") and open for the second (\"b\") — for instance, 30° of longitude is the first point of Taurus, not part of Aries. Association of calendar dates with astrological signs only makes sense when referring to Sun sign astrology.\n"
                              "Empedocles, a fifth-century BC Greek philosopher, identified Fire, Earth, Air, and Water as elements. He explained the nature of the universe as an interaction of two opposing principles called love and strife manipulating the four elements, and stated that these four elements were all equal, of the same age, that each rules its own province, and each possesses its own individual character. Different mixtures of these elements produced the different natures of things. Empedocles said that those who were born with near equal proportions of the four elements are more intelligent and have the most exact perceptions.\n"
                              ;
    auto result = test::compress<lz77::LZ77DoubleHashing<lz77::LZ77StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(input, "HASH_BITS=15");
    decompress<lz77::LZ77DoubleHashing<lzss::StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(result.str, input, "");
}
TEST(LZ77DoubleHashing, MODE_2) {
    const std::string input = "In Western astrology, astrological signs are the twelve 30° sectors of the ecliptic, starting at the vernal equinox (one of the intersections of the ecliptic with the celestial equator), also known as the First Point of Aries. The order of the astrological signs is Aries, Taurus, Gemini, Cancer, Leo, Virgo, Libra, Scorpio, Sagittarius, Capricorn, Aquarius and Pisces.\n"
                              "The concept of the zodiac originated in Babylonian astrology, and was later influenced by Hellenistic culture. According to astrology, celestial phenomena relate to human activity on the principle of \"as above, so below\", so that the signs are held to represent characteristic modes of expression.\n"
                              "The twelve sector division of the ecliptic constitutes astrology's primary frame of reference when considering the positions of celestial bodies, from a geocentric point of view, so that we may find, for instance, the Sun in 23° \"Aries\" (23° longitude), the Moon in 7° \"Scorpio\" (217° longitude), or Jupiter in 29° \"Pisces\" (359° longitude). Beyond the celestial bodies, other astrological points that are dependent on geographical location and time (namely, the Ascendant, the Midheaven, the Vertex and the houses' cusps) are also referenced within this ecliptic coordinate system.\n"
                              "Various approaches to measuring and dividing the sky are currently used by differing systems of astrology, although the tradition of the Zodiac's names and symbols remain consistent. Western astrology measures from Equinox and Solstice points (points relating to equal, longest and shortest days of the tropical year), while Jyotiṣa or Vedic astrology measures along the equatorial plane (sidereal year). Precession results in Western astrology's zodiacal divisions not corresponding in the current era to the constellations that carry similar names, while Jyotiṣa measurements still correspond with the background constellations.\n"
                              "In Western and Asian astrology, the emphasis is on space, and the movement of the Sun, Moon and planets in the sky through each of the zodiac signs. In Chinese astrology, by contrast, the emphasis is on time, with the zodiac operating on cycles of years, months, and hours of the day.\n"
                              "A common feature of all three traditions however, is the significance of the Ascendant — the zodiac sign that is rising (due to the rotation of the earth) on the eastern horizon at the moment of a person's birth.\n"
                              "While Western astrology is essentially a product of Greco-Roman culture, some of its more basic concepts originated in Babylonia. Isolated references to celestial \"signs\" in Sumerian sources are insufficient to speak of a Sumerian zodiac. Specifically, the division of the ecliptic in twelve equal sectors is a Babylonian conceptual construction.\n"
                              "By the 4th century BC, Babylonians' astronomy and their system of celestial omens were influencing the Greek culture and, by the late 2nd century BC, Egyptian astrology was also mixing in. This resulted, unlike the Mesopotamian tradition, in a strong focus on the birth chart of the individual and in the creation of horoscopic astrology, employing the use of the Ascendant (the rising degree of the ecliptic, at the time of birth), and of the twelve houses. Association of the astrological signs with Empedocles' four classical elements was another important development in the characterization of the twelve signs.\n"
                              "The body of astrological knowledge by the 2nd century AD is described in Ptolemy's \"Tetrabiblos\", a work that was responsible for astrology's successful spread across Europe and the Middle East, and remained a reference for almost seventeen centuries as later traditions made few substantial changes to its core teachings.\n"
                              "The following table enumerates the twelve divisions of celestial longitude, with the Latin names (still widely used) and the English translation (gloss). The longitude intervals, being a mathematical division, are closed for the first endpoint (\"a\") and open for the second (\"b\") — for instance, 30° of longitude is the first point of Taurus, not part of Aries. Association of calendar dates with astrological signs only makes sense when referring to Sun sign astrology.\n"
                              "Empedocles, a fifth-century BC Greek philosopher, identified Fire, Earth, Air, and Water as elements. He explained the nature of the universe as an interaction of two opposing principles called love and strife manipulating the four elements, and stated that these four elements were all equal, of the same age, that each rules its own province, and each possesses its own individual character. Different mixtures of these elements produced the different natures of things. Empedocles said that those who were born with near equal proportions of the four elements are more intelligent and have the most exact perceptions.\n"
                              ;
    auto result = test::compress<lz77::LZ77DoubleHashing<lz77::LZ77StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(input, "HASH_BITS=15,COMPRESSION_MODE=2");
    decompress<lz77::LZ77DoubleHashing<lzss::StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(result.str, input, "");
}

TEST(LZ77DoubleHashing, MODE_1_DID) {
    const std::string input = "In Western astrology, astrological signs are the twelve 30° sectors of the ecliptic, starting at the vernal equinox (one of the intersections of the ecliptic with the celestial equator), also known as the First Point of Aries. The order of the astrological signs is Aries, Taurus, Gemini, Cancer, Leo, Virgo, Libra, Scorpio, Sagittarius, Capricorn, Aquarius and Pisces.\n"
                              "The concept of the zodiac originated in Babylonian astrology, and was later influenced by Hellenistic culture. According to astrology, celestial phenomena relate to human activity on the principle of \"as above, so below\", so that the signs are held to represent characteristic modes of expression.\n"
                              "The twelve sector division of the ecliptic constitutes astrology's primary frame of reference when considering the positions of celestial bodies, from a geocentric point of view, so that we may find, for instance, the Sun in 23° \"Aries\" (23° longitude), the Moon in 7° \"Scorpio\" (217° longitude), or Jupiter in 29° \"Pisces\" (359° longitude). Beyond the celestial bodies, other astrological points that are dependent on geographical location and time (namely, the Ascendant, the Midheaven, the Vertex and the houses' cusps) are also referenced within this ecliptic coordinate system.\n"
                              "Various approaches to measuring and dividing the sky are currently used by differing systems of astrology, although the tradition of the Zodiac's names and symbols remain consistent. Western astrology measures from Equinox and Solstice points (points relating to equal, longest and shortest days of the tropical year), while Jyotiṣa or Vedic astrology measures along the equatorial plane (sidereal year). Precession results in Western astrology's zodiacal divisions not corresponding in the current era to the constellations that carry similar names, while Jyotiṣa measurements still correspond with the background constellations.\n"
                              "In Western and Asian astrology, the emphasis is on space, and the movement of the Sun, Moon and planets in the sky through each of the zodiac signs. In Chinese astrology, by contrast, the emphasis is on time, with the zodiac operating on cycles of years, months, and hours of the day.\n"
                              "A common feature of all three traditions however, is the significance of the Ascendant — the zodiac sign that is rising (due to the rotation of the earth) on the eastern horizon at the moment of a person's birth.\n"
                              "While Western astrology is essentially a product of Greco-Roman culture, some of its more basic concepts originated in Babylonia. Isolated references to celestial \"signs\" in Sumerian sources are insufficient to speak of a Sumerian zodiac. Specifically, the division of the ecliptic in twelve equal sectors is a Babylonian conceptual construction.\n"
                              "By the 4th century BC, Babylonians' astronomy and their system of celestial omens were influencing the Greek culture and, by the late 2nd century BC, Egyptian astrology was also mixing in. This resulted, unlike the Mesopotamian tradition, in a strong focus on the birth chart of the individual and in the creation of horoscopic astrology, employing the use of the Ascendant (the rising degree of the ecliptic, at the time of birth), and of the twelve houses. Association of the astrological signs with Empedocles' four classical elements was another important development in the characterization of the twelve signs.\n"
                              "The body of astrological knowledge by the 2nd century AD is described in Ptolemy's \"Tetrabiblos\", a work that was responsible for astrology's successful spread across Europe and the Middle East, and remained a reference for almost seventeen centuries as later traditions made few substantial changes to its core teachings.\n"
                              "The following table enumerates the twelve divisions of celestial longitude, with the Latin names (still widely used) and the English translation (gloss). The longitude intervals, being a mathematical division, are closed for the first endpoint (\"a\") and open for the second (\"b\") — for instance, 30° of longitude is the first point of Taurus, not part of Aries. Association of calendar dates with astrological signs only makes sense when referring to Sun sign astrology.\n"
                              "Empedocles, a fifth-century BC Greek philosopher, identified Fire, Earth, Air, and Water as elements. He explained the nature of the universe as an interaction of two opposing principles called love and strife manipulating the four elements, and stated that these four elements were all equal, of the same age, that each rules its own province, and each possesses its own individual character. Different mixtures of these elements produced the different natures of things. Empedocles said that those who were born with near equal proportions of the four elements are more intelligent and have the most exact perceptions.\n"
                              ;
    auto result = test::compress<lz77::LZ77DoubleHashing<lzss::DidacticalCoder>>(input, "HASH_BITS=15,COMPRESSION_MODE=1");
    ASSERT_EQ(result.str, "In Western astrology,{11, 9}ical signs are th{4, 3}welve 30\\xC2\\xB0 sectors of{27, 5}ecliptic, starting at{53, 5}v{96, 3}{74, 3}equinox (one{53, 8}in{125, 3}{73, 4}io{102, 3}{21, 7}{74, 8} with{66, 5}cel{167, 3}i{69, 6}a{118, 3}){166, 3}lso know{188, 4}{104, 5}First Po{85, 3}{75, 4}Aries. T{64, 3}order{95, 8}{223, 19}is{46, 6}, Tauru{8, 3}Gemini, Cancer, Leo, Virg{7, 3}Libra, Scorpi{16, 3}Sagit{244, 3}i{57, 4}Capri{26, 3}n, A{172, 3}{21, 4} and Pisc{143, 3}\\n{143, 4}co{85, 3}p{165, 5}{184, 4}zodiac{165, 3}iginated{279, 3} Babylonia{223, 4}{177, 6}{411, 4}{74, 3}w{239, 3}l{39, 3}r{318, 3}flue{76, 3}d by Helleni{296, 3}c culture. Ac{166, 3}d{397, 4}to{73, 12}{338, 10}phenomena re{87, 4}{41, 4}hum{120, 4}{407, 3}vity on{509, 5}{217, 3}ncipl{336, 3}f \\\"{134, 3}above{496, 3}o below\\\"{11, 5}th{503, 7}{347, 6}{570, 4}held{87, 4}represe{415, 3}char{95, 3}er{175, 6}mode{511, 5}ex{34, 4}s{526, 3}{298, 6}{624, 7}{546, 4}or d{143, 3}{28, 4}{313, 8}{553, 9}{337, 3}{205, 3}tut{70, 3}{229, 9}'s{178, 4}mary fram{182, 5}refer{306, 4} w{247, 3}{56, 5}i{539, 3}{288, 5}{226, 4}osi{651, 9}{291, 10}b{417, 3}{541, 4}from a geoc{197, 3}{487, 3}{49, 3}{621, 7}view{255, 10}we ma{119, 3}ind{54, 3}{186, 3}i{160, 3}{586, 4},{282, 5}Sun{483, 4}23{838, 3}\\\"A{633, 4}\\\" ({14, 5}{498, 3}{588, 3}ude{737, 3}{226, 4}Mo{380, 3}{43, 3}7{42, 4}{625, 7}{44, 4}1{17, 4}{45, 12}{286, 3}Jupi{534, 6} 29{48, 4}{627, 6}{47, 3}35{16, 4}{47, 10}. Beyo{661, 3}{141, 4}{231, 18}o{267, 3}r{328, 9}{807, 5}{232, 5}{870, 4}{476, 3}{466, 4}depend{458, 4}{398, 3}{272, 3}graph{42, 5}loca{323, 4}{683, 5}ti{375, 3}(n{381, 3}l{701, 3}{209, 4}A{147, 3}ndant{15, 6}Midheave{814, 3}{962, 4}Vertex{59, 6}{510, 3}houses'{718, 3}sps){121, 5}{1014, 5}{454, 9}{781, 3}{1059, 3}{241, 3}{5, 3}s{527, 12}{756, 5}{717, 4}sy{1247, 3}m.\\nVari{78, 3} approach{553, 3}{785, 3}measu{510, 5}{115, 4}{607, 4}{811, 6}{702, 4}ky{113, 5}cur{107, 3}tly {140, 3}{872, 5}dif{127, 3}{53, 4}{93, 6}{557, 5}{302, 8}{932, 4}lt{185, 3}g{1210, 6}trad{595, 5}{692, 8}Z{357, 3}ac{668, 3}{278, 4}{799, 3}{977, 3}symbols{799, 3}ma{492, 3}{661, 5}{1429, 3}nt.{1438, 18}{179, 7}{814, 3}{653, 5}E{1364, 7}{71, 4}S{68, 3}{254, 3}{710, 4}{429, 5}({8, 7}{983, 5}{215, 5}o{1343, 5}l,{523, 5}{90, 3}{55, 5}sho{373, 3}{1337, 3}day{203, 5}{183, 6}op{463, 5}yea{1389, 4}whi{1015, 3}Jyoti\\xE1\\xB9\\xA3\" \"a{623, 4}Ved{954, 3}{153, 19}a{94, 4}{478, 5}{110, 4}{1453, 3}{604, 4}pla{1526, 3}({878, 5}e{88, 8}. Pre{675, 3}{980, 5}{1050, 3}{1205, 3}{1420, 3}n{246, 18}{303, 3}z{312, 5}{74, 3}{422, 4}{49, 4}{322, 3}o{1097, 3}o{415, 3}sp{717, 3}{454, 4}{522, 5}{436, 9} era{245, 4}{742, 5}{1058, 4}{1313, 3}{668, 5}{713, 7}car{1052, 3}similar{400, 6}{234, 18}{215, 7}{1313, 3}{331, 3}{1125, 3}ll{76, 3}{116, 8}{639, 5}{471, 5}backgrou{392, 3}{107, 14}.\\nI{209, 11}{614, 3}As{1492, 15}{298, 5}mpha{502, 3}{1674, 4}{850, 3}spa{1068, 4}{661, 4}{821, 4}m{1383, 3}{129, 4}{414, 8}{1088, 3},{1051, 6}{34, 4}{347, 5}{313, 6}{234, 4}{697, 4}th{139, 3}{642, 3}e{746, 3}{53, 8}{321, 6}{1430, 6}. {149, 3}C{825, 3}{1422, 3}{139, 12}{734, 3}{190, 3}{692, 3}s{928, 7}{152, 15}{976, 4}{291, 3}{245, 8}{83, 7}op{361, 3}{610, 5}{1037, 3}cyc{1104, 3}{787, 4}{483, 4}{1874, 3}m{81, 3}h{8, 3}{171, 4}{792, 3}{2097, 10}{624, 3}.\\nA{941, 3}m{35, 3} fea{1714, 4}{57, 4}a{356, 3}{194, 3}e{647, 4}{830, 7}s{59, 3}we{2121, 3},{138, 4}{260, 4}{200, 4}if{676, 3}{1031, 3}{82, 8}{1117, 9} \\xE2\\x80\\x94{162, 12}{45, 4}{116, 3}{1213, 3}{200, 3}{1652, 3}{782, 4}(du{1632, 3}{542, 6}rot{430, 5}{73, 8}{669, 3}th){207, 4}{14, 6}{446, 6}{809, 3}iz{373, 4}t{23, 5}m{1846, 4}{404, 5}a {258, 3}son{679, 3}bi{58, 3}.\\nW{582, 5}{503, 9}{354, 8}{127, 4}{751, 3}{668, 3}{788, 3}{1107, 3}{57, 3}roduc{70, 5}G{780, 3}o-Ro{1915, 4}{1984, 7}{1618, 4}{1721, 6}i{481, 3}mo{1168, 3}b{399, 3}{1250, 4}{247, 3}p{658, 3}{2101, 23}{481, 3}so{642, 3}{1202, 3}{1325, 9}{1266, 5}{1518, 10}\\\"{277, 4}{1566, 3}{1138, 3}Sum{1930, 3}{119, 3}s{416, 3}{41, 4}{1273, 4}{1715, 3}uf{353, 3}i{842, 4}{58, 3}speak{139, 4}a{48, 10}{351, 6}. Spec{397, 5}{213, 3},{308, 5}{934, 8}{338, 9}{1436, 8}{678, 4}{2005, 6}{1054, 4}{2433, 3}{2011, 5}{609, 5}a{192, 10}{1286, 5}{226, 4}{35, 4}{11, 3}{321, 3}{294, 3}{2071, 5}B{735, 4}e 4{644, 3}{494, 3}{294, 3}y BC,{59, 11}{1585, 3}{369, 5}{2262, 3}{354, 3}{1670, 4}heir{1453, 7}{361, 4}{260, 10}{450, 4}s{1966, 3}{239, 5}{2380, 6}{539, 4}{1073, 4}{398, 3}ek{392, 8}{71, 4}{798, 5}{219, 4}{343, 4} 2{1021, 3}{127, 12}Egy{223, 3}{985, 12}{2472, 5}{1711, 5}mix{1181, 6}{2701, 4}{641, 4}{1258, 5}e{82, 3}unlik{2906, 6}Mesop{647, 3}m{69, 4}{760, 9}{751, 3}{624, 3}{1144, 3}{207, 3}g{2122, 3}{1802, 3}{662, 8}{618, 5}{2377, 5}{574, 5}{2894, 6}{375, 4}d{308, 4}{1130, 4}{1041, 7}c{1393, 3}{76, 4}{38, 4}{707, 3}osc{1500, 4}{169, 10},{998, 4}loy{250, 8}{1768, 3}{43, 4}{11, 4}{849, 10}({115, 4}{826, 7}deg{925, 4}{36, 7}{474, 8}{1785, 3}{792, 6}{1061, 4}{2323, 4}{161, 5}{2985, 4}{1300, 3}{44, 7}{506, 7}{1823, 3}{2008, 3}{2717, 3}ssoci{157, 9}{78, 4}{150, 8}{588, 4}{951, 5}{415, 3}{1135, 4}Empedo{1115, 4}'{263, 3}ur cla{1592, 3}{2162, 4}{462, 3}{905, 4}{41, 3}{369, 4}{1557, 3}{2238, 4}imp{1751, 3}{186, 4}d{1079, 3}lop{35, 4}{274, 9}{2686, 9}z{125, 13}{159, 7}{119, 5}{2688, 6}{2323, 3}{2815, 3}{2015, 10}{113, 5}{3189, 4}ledg{388, 3}{530, 6}{525, 12}AD{715, 4}{2767, 3}crib{918, 6}Pto{163, 3}{1733, 4}\\\"Te{1373, 3}biblos{2862, 3}a work{123, 3}{1677, 3}{189, 4}{1622, 6}s{31, 3}e{2614, 5}{118, 8}{59, 4}suc{1834, 4}fu{819, 3}p{466, 3}d{2975, 3}{460, 3}s Eu{1964, 3}{674, 5}{321, 5}{2386, 3}d{1147, 3}E{1475, 5}{379, 4}{1724, 3}a{1513, 3}{48, 3}{1035, 10}{94, 6}lmo{2052, 3}s{288, 3}{3462, 3}{2830, 4}{195, 5}{2563, 3}{3409, 4}{742, 4}r{649, 10}{1150, 3}ad{56, 3}ew{138, 3}b{2765, 4}{833, 5}{330, 3}{2122, 4}{1056, 4}{1186, 4}{2721, 3}{512, 3}{1654, 4}{1550, 3}{323, 7}fol{3100, 3}{568, 4}ta{217, 4}en{1086, 4}{94, 3}{1480, 6}{372, 7}{697, 4}{1072, 4}{735, 3}{929, 12}{2123, 4}{2740, 5}{1654, 11}L{1644, 4}{1967, 6}{2129, 3}{1938, 5}w{2135, 3}{1359, 3}{698, 3}d{2599, 3}{266, 7}Englis{52, 3}r{1044, 3}l{489, 6}(g{378, 3}s{2169, 3}{477, 4}{93, 9}{825, 3}{1448, 3}v{941, 3}{997, 3}e{940, 4}{2040, 3}a{1079, 3}{5, 3}{498, 5}{158, 8}{338, 3}{2579, 4}{68, 3}{1369, 3}{324, 4}{790, 4}f{3703, 5}{808, 3}{2416, 5} (\\\"a\\\"{133, 6}{407, 3}{1752, 3}{38, 7}{1256, 3}{2097, 4}(\\\"b{30, 3}{1695, 4}{25, 4}{1380, 3}{3095, 7}{3922, 5}{929, 3}{148, 11}{288, 6}{94, 6}{91, 6}{679, 3}{3744, 8}{743, 3} p{1019, 7}{3138, 5}{846, 17}{682, 3}{956, 4}{3382, 3}{364, 5}{319, 5}{593, 8}{213, 5}{743, 5}{1102, 3}{313, 3}mak{35, 3}{1688, 3}{1019, 3}{3353, 5}{554, 5}{2788, 5}{1530, 3}{2159, 3}{45, 5}{63, 9}{1976, 3}{912, 10}{711, 4}fifth-{576, 6}{1296, 4}{1342, 7}{3091, 3}{367, 3}o{3682, 3}{1976, 4}{3119, 4}{1573, 3}{314, 3}Fi{1755, 4}E{191, 3}h{3878, 3}i{29, 3}{303, 4}W{626, 5}{964, 3}{977, 8}.{3795, 3}{3602, 4}l{704, 6}{270, 4}{1770, 3}{1423, 4}{944, 7}uni{4188, 3}{2243, 5}{66, 3}{454, 6}{984, 3}{257, 7}two{387, 3}{3539, 4}{199, 3}{3772, 9}{677, 3}{1702, 3}{79, 3}l{2389, 3}{120, 5}{1628, 3}if{3505, 4}nipu{552, 4}{1288, 7}{1127, 5}{140, 8}{163, 6}{430, 3}{1464, 3}{646, 3}{1259, 6}{302, 3}{37, 13}{1610, 6}{2239, 4}{1756, 5}{3478, 3}{168, 6}s{3329, 3} ag{3582, 5}{2180, 3}{800, 4} ru{140, 4}{821, 4}{4290, 4}{2055, 3}v{161, 3}{278, 3}{1465, 4}{33, 5}{185, 3}{3325, 3}{1320, 3}{37, 9}{1503, 11}{888, 3}{234, 4}er. D{3215, 5}{1246, 4}{1640, 3}{286, 4}{468, 3}{119, 5}{280, 3}{153, 9}{2151, 6}{187, 5}{876, 4}{50, 8}{335, 6}{49, 7}{947, 5}{1385, 11}{183, 3}i{49, 4}{236, 5}{754, 3}{538, 3}o{226, 6}b{4319, 3}{593, 6}n{3103, 3}{237, 6}{99, 4}{1395, 4}{964, 8}{322, 17}{827, 5}{2243, 5}{424, 4}llig{180, 4}{941, 4}h{3585, 3}{464, 5}{1168, 5}ex{217, 3}{2385, 4}{2047, 4}{2882, 6}");

}