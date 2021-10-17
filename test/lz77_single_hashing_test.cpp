#include <tudocomp/compressors/lz77/LZ77SingleHashing.hpp>
#include "test/util.hpp"

using namespace tdc;

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

TEST(LZ77SingleHashing, DidacticalSimpleTest) {
    const std::string input = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    auto result = test::compress<lz77::LZ77SingleHashing<lzss::DidacticalCoder>>(input, "HASH_BITS=4");
    ASSERT_EQ(result.str, "cbabc{3, 5}{3, 5}bc{16, 5}bccbabc{3, 5}{3, 5}bcbabc{3, 3}");
}

TEST(LZ77SingleHashing, DecompressAsciiCode) {
    const std::string orig = "cbabcabcabcabcabcbabcabccbabcabcabcabcabcbabcabc";
    const std::string input = "3:5:0c0b0a0b0c13:5:13:5:0b0c116:5:0b0c0c0b0a0b0c13:5:13:5:0b0c0b0a0b0c13:3";
    decompress<lz77::LZ77SingleHashing<lz77::LZ77StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(input, orig,
                                                                                                      "HASH_BITS=4");
}

TEST(LZ77SingleHashing, ExampleWikiText) {
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
                              "Empedocles, a fifth-century BC Greek philosopher, identified Fire, Earth, Air, and Water as elements. He explained the nature of the universe as an interaction of two opposing principles called love and strife manipulating the four elements, and stated that these four elements were all equal, of the same age, that each rules its own province, and each possesses its own individual character. Different mixtures of these elements produced the different natures of things. Empedocles said that those who were born with near equal proportions of the four elements are more intelligent and have the most exact perceptions.\n";
    auto result = test::compress<lz77::LZ77SingleHashing<lz77::LZ77StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(
            input, "HASH_BITS=15");
    decompress<lz77::LZ77SingleHashing<lz77::LZ77StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(result.str, input,
                                                                                                      "");
}

TEST(COMPRESSION_MODE_TEST, MODE_1) {
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
                              "Empedocles, a fifth-century BC Greek philosopher, identified Fire, Earth, Air, and Water as elements. He explained the nature of the universe as an interaction of two opposing principles called love and strife manipulating the four elements, and stated that these four elements were all equal, of the same age, that each rules its own province, and each possesses its own individual character. Different mixtures of these elements produced the different natures of things. Empedocles said that those who were born with near equal proportions of the four elements are more intelligent and have the most exact perceptions.\n";
    auto result = test::compress<lz77::LZ77SingleHashing<lz77::LZ77StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(input,
                                                                                 "HASH_BITS=8, COMPRESSION_MODE=1");
    result.assert_decompress();
}

TEST(COMPRESSION_MODE_TEST, MODE_2) {
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
                              "Empedocles, a fifth-century BC Greek philosopher, identified Fire, Earth, Air, and Water as elements. He explained the nature of the universe as an interaction of two opposing principles called love and strife manipulating the four elements, and stated that these four elements were all equal, of the same age, that each rules its own province, and each possesses its own individual character. Different mixtures of these elements produced the different natures of things. Empedocles said that those who were born with near equal proportions of the four elements are more intelligent and have the most exact perceptions.\n";
    auto result = test::compress<lz77::LZ77SingleHashing<lz77::LZ77StreamingCoder<ASCIICoder, ASCIICoder, ASCIICoder>>>(input,
                                                                                                                        "HASH_BITS=8, COMPRESSION_MODE=2");
    result.assert_decompress();
}

TEST(HASH_TEST, MODE_2) {
    const int hashTableSize = 20;

    uint * hashTableHead = new uint[hashTableSize];
    uint * hashTablePrev = new uint[hashTableSize];
    memset(hashTableHead, 0, sizeof(uint) * hashTableSize);
    memset(hashTablePrev, 0, sizeof(uint) * hashTableSize);

    const std::string input = "Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet. Lorem ipsum dolor sit amet, consetetur sadipscing elitr, sed diam nonumy eirmod tempor invidunt ut labore et dolore magna aliquyam erat, sed diam voluptua. At vero eos et accusam et justo duo dolores et ea rebum. Stet clita kasd gubergren, no sea takimata sanctus est Lorem ipsum dolor sit amet.";
    std::cout << "Länge der Eingabe: "<< input.length() << std::endl;
    std::cout << "Hash table:" << std::endl;

    uint MIN_MATCH = 3;
    uint length = input.size();
    const uint maxLen = length - MIN_MATCH;
    uint hashHead;
    uint HASH_MASK = hashTableSize - 1;
    uint HASH_BITS = 9;
    uint H_SHIFT = (HASH_BITS + MIN_MATCH - 1) / MIN_MATCH;
    for(int positionInText = 0; positionInText < maxLen; positionInText++){
        hashHead = 0;
        for (uint j = 0; j < MIN_MATCH; j++) {
            hashHead = ((hashHead << H_SHIFT) ^ input[j + positionInText]) & HASH_MASK;
        }
        hashTablePrev[positionInText & HASH_MASK] = hashTableHead[hashHead];
        hashTableHead[hashHead] = positionInText;
    }

    for(int i = 0; i < hashTableSize; i++) {
        std::cout << "hashTable[i]=" << hashTableHead[i] << std::endl;
    }
}