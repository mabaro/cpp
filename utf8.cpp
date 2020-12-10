/*
https://fasterthanli.me/articles/working-with-strings-in-rust
https://www.unicode.mayastudios.com/examples/utf8.html
http://www.columbia.edu/~fdc/utf8/
https://www.branah.com/unicode-converter
*/
#include <cstdio>
#include <cstdint>
#include <string>
#include <iostream>
#include <vector>
#include <bitset>
#include <cassert>

 
namespace detail
{
/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

///@return utf32 codepoint for the first utf8 codepoint found, uint32_t(-1) when invalid
uint32_t utf8_to_utf32(const uint8_t* i_utf8Str, size_t* o_numBytes = nullptr)
{
	constexpr uint32_t lowest6bit0 = 0b000000000000000000111111;
	constexpr uint32_t lowest6bit1 = 0b000000000000111111000000;
	constexpr uint32_t lowest6bit2 = 0b000000111111000000000000;

	const uint8_t* srcPtr = i_utf8Str;
	const uint8_t c = *srcPtr++;

	auto setNumBytes = [o_numBytes](size_t count) {
		if (o_numBytes)
		{
			*o_numBytes = count;
		}
	};

	uint32_t output(-1);

	if (!(c & 0b10000000))        // 1-byte sequence
	{
		output = (uint32_t)c;
		setNumBytes(1);
	}
	else if (c >> 5 == 0b110)     // 2-byte sequence --> 0b110xxxxx10xxxxxx
	{
		const uint32_t b1 = (uint32_t)c;
		const uint32_t b2 = (uint32_t) * (srcPtr++);
		const uint32_t highestByte = 0b0000011111000000;

		output = ((b1 << 6) & highestByte) | ((b2 << 0) & lowest6bit0);
		setNumBytes(2);
	}
	else if (c >> 4 == 0b1110)    // 3-byte sequence --> 0b1110xxxx10xxxxxx10xxxxxx
	{
		const uint32_t b1 = (uint32_t)c;
		const uint32_t b2 = (uint32_t) * (srcPtr++);
		const uint32_t b3 = (uint32_t) * (srcPtr++);
		const uint32_t highestByte = 0b1111000000000000;

		output = ((b1 << 12) & highestByte) | ((b2 << 6) & lowest6bit1) | ((b3 << 0) & lowest6bit0);
		setNumBytes(3);
	}
	else if ((c >> 3) == 0b11110) // 4-byte sequence --> 0b11110xxx10xxxxxx10xxxxxx10xxxxxx
	{
		const uint32_t b1 = (uint32_t)c;
		const uint32_t b2 = (uint32_t) * (srcPtr++);
		const uint32_t b3 = (uint32_t) * (srcPtr++);
		const uint32_t b4 = (uint32_t) * (srcPtr++);
		const uint32_t highestByte = 0b111000000000000000000;

		output = ((b1 << 18) & highestByte) | ((b2 << 12) & lowest6bit2) | ((b3 << 6) & lowest6bit1) | ((b4 << 0) & lowest6bit0);
		setNumBytes(4);
	}
	else
	{
		assert(false);
        std::cerr << "invalid codepoint: " <<  i_utf8Str << std::endl;
	}

	return output;
}

///@return numBytes of the utf8 code, = 0 if the codepoint is invalid, = -numBytes if the output length is less than required to decode
int32_t utf32_to_utf8(const uint32_t codePoint, char* o_src, const size_t i_srcLen)
{
	const uint32_t mask1 = (1 << 7) - 1;  //7bit
	const uint32_t mask2 = (1 << 11) - 1; //11bit
	const uint32_t mask3 = (1 << 16) - 1; //16bit
	const uint32_t mask4 = (1 << 21) - 1; //21bit
	const uint32_t k_continuationMarker = 0b10000000;
	const uint32_t k_6bitMask           = 0b00111111;

	int32_t numBytes = 0;

	if (codePoint > 0x1FFFFF)
	{
        std::cerr << "invalid utf32 codepoint: " << codePoint << std::endl;
		assert(false);
		return 0;
	}

	if (!(codePoint & ~0b1111111))
	{
        o_src[0] = codePoint;
		numBytes = 1;
	}
	else if (!(codePoint & ~mask2)) // 0b110xxxxx
	{
		numBytes = 2;
		if (i_srcLen < numBytes)
		{
            std::cerr << i_srcLen <<  numBytes << std::endl;
			return -numBytes;
		}
		o_src[0] = 0b11000000 | (0b00011111 & (codePoint >> 6));
		o_src[1] = k_continuationMarker | (k_6bitMask & (codePoint));
	}
	else if (!(codePoint & ~mask3)) // 0b1110xxxx
	{
		numBytes = 3;
		if (i_srcLen < numBytes)
		{
			return -numBytes;
		}
		o_src[0] = 0b11100000 | (0b00001111 & (codePoint >> 12));
		o_src[1] = k_continuationMarker | (k_6bitMask & (codePoint >> 6));
		o_src[2] = k_continuationMarker | (k_6bitMask & (codePoint));
	}
	else if (!(codePoint & ~mask4)) // 0b11110xxx
	{
		numBytes = 4;
		if (i_srcLen < numBytes)
		{
			return -numBytes;
		}

		o_src[0] = 0b11110000 | (0b00000111 & (codePoint >> 18));
		o_src[1] = k_continuationMarker | (k_6bitMask & (codePoint >> 12));
		o_src[2] = k_continuationMarker | (k_6bitMask & (codePoint >> 6));
		o_src[3] = k_continuationMarker | (k_6bitMask & (codePoint));
	}

	return numBytes;
}

///@return num bytes of next code
size_t peek_utf8(const uint8_t* i_str)
{
	const uint8_t  c = *i_str;

	if (!(c & 0b10000000))        // 1-byte sequence
	{
		return 1;
	}
	else if (c >> 5 == 0b110)     // 2-byte sequence --> 0b110xxxxx10xxxxxx
	{
		return 2;
	}
	else if (c >> 4 == 0b1110)    // 3-byte sequence --> 0b1110xxxx10xxxxxx10xxxxxx
	{
		return 3;
	}
	else if ((c >> 3) == 0b11110) // 4-byte sequence --> 0b11110xxx10xxxxxx10xxxxxx10xxxxxx
	{
		return 4;
	}
	else
	{
		assert(false);// "invalid codepoint: {}", *i_str
	}

	return 0;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

bool encode_utf8(const uint32_t codePoint, std::string& output) {
    const uint32_t mask1 = (1 << 7) - 1; //7bit
    const uint32_t mask2 = (1 << 11) - 1; //11bit
    const uint32_t mask3 = (1 << 16) - 1; //16bit
    const uint32_t mask4 = (1 << 21) - 1; //21bit
    const uint32_t k_continuationMarker = 0b10000000;
    const uint32_t k_continuationMask   = 0b10111111;

    if (codePoint > 0x1FFFFF) {
        std::cerr << "invalid code point: " << codePoint << std::endl;
        return false;
    }

    if (!(codePoint & ~0b1111111))
    {
        output.push_back(codePoint);
    }
    else if (!(codePoint & ~mask2))
    {
        output.reserve(output.size() + 1);
        output.push_back(0b11000000 | (0b00011111 & (codePoint >> 6)));
        output.push_back(k_continuationMarker | (k_continuationMask & (codePoint)));
    }
    else if (!(codePoint & ~mask3))
    {
        output.reserve(output.size() + 2);
        output.push_back(0b11100000 | (0b00001111 & (codePoint >> 12)));
        output.push_back(k_continuationMarker | (k_continuationMask & (codePoint >> 6)));
        output.push_back(k_continuationMarker | (k_continuationMask & (codePoint)));
    }
    else if (!(codePoint & ~mask4))
    {
        output.reserve(output.size() + 3);
        output.push_back(0b11110000 | (0b00000111 & (codePoint >> 18)));
        output.push_back(k_continuationMarker | (k_continuationMask & (codePoint >> 12)));
        output.push_back(k_continuationMarker | (k_continuationMask & (codePoint >> 6)));
        output.push_back(k_continuationMarker | (k_continuationMask & (codePoint )));
    }

    return true;
}

/////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////
}//detail


bool encode_utf32_to_utf8(const std::vector<uint32_t>& src, std::string& output) {
    output.reserve(src.size());

    bool isValid = true;
    for(auto it = std::begin(src), lastIt = std::end(src);
        it != lastIt && *it != 0 && isValid;
        ++it)
    {
        isValid = detail::encode_utf8(*it, output);
    }

    output.shrink_to_fit();

    return isValid;
 }


bool encode_utf8_to_utf32(const char *src, std::vector<uint32_t>& dst) {
    const uint8_t* srcPtr = (uint8_t*)src;

    while (*srcPtr != 0) {
        size_t numBytes;
        const uint32_t scalar = detail::utf8_to_utf32(srcPtr, &numBytes);
        dst.push_back(scalar);

        srcPtr += numBytes;
    }

    return true;
}


////////////////////////////////////////////////////////////////////////////////////////////////
////////////////////////////////////////////////////////////////////////////////////////////////

int main(int argc, char **argv) {
    if (1)
    {// simple multi-byte test
        std::string temp = "𤭢€¢$"; //4,3,2,1bytes

        std::cout << "original: " << temp << std::endl;

        std::vector<uint32_t> decoded;
        bool isOk = encode_utf8_to_utf32(temp.c_str(), decoded);
        std::cout << "decoded : ";
        for(uint32_t u32 : decoded)
        {
            std::cout << std::hex << u32;
        }
        std::cout << std::endl;

        std::string encoded;
        isOk = encode_utf32_to_utf8(decoded, encoded);
        std::cout << "encoded : " << encoded << std::endl;
        assert(encoded.size() == temp.size());
        assert(encoded == temp);
    }

    std::pair<std::string, std::string> samples[] = {
{ "Sanskrit", "﻿काचं शक्नोम्यत्तुम् । नोपहिनस्ति माम् ॥"},
{ "Sanskrit (standard transcription)", "kācaṃ śaknomyattum; nopahinasti mām."},
{ "Classical Greek", "ὕαλον ϕαγεῖν δύναμαι· τοῦτο οὔ με βλάπτει."},
{ "Greek (monotonic)", "Μπορώ να φάω σπασμένα γυαλιά χωρίς να πάθω τίποτα."},
{ "Greek (polytonic)", "Μπορῶ νὰ φάω σπασμένα γυαλιὰ χωρὶς νὰ πάθω τίποτα."},
{ "Etruscan", "(NEEDED)"},
{ "Latin", "Vitrum edere possum; mihi non nocet."},
{ "Old French", "Je puis mangier del voirre. Ne me nuit."},
{ "French", "Je peux manger du verre, ça ne me fait pas mal."},
{ "Provençal / Occitan", "Pòdi manjar de veire, me nafrariá pas."},
{ "Québécois", "J'peux manger d'la vitre, ça m'fa pas mal."},
{ "Walloon", "Dji pou magnî do vêre, çoula m' freut nén må."},
{ "Champenois", "(NEEDED)"},
{ "Lorrain", "(NEEDED)"},
{ "Picard", "Ch'peux mingi du verre, cha m'foé mie n'ma."},
{ "Corsican/Corsu", "(NEEDED)"},
{ "Jèrriais", "(NEEDED)"},
{ "Kreyòl Ayisyen (Haitï)", "Mwen kap manje vè, li pa blese'm."},
{ "Basque", "Kristala jan dezaket, ez dit minik ematen."},
{ "Catalan / Català", "Puc menjar vidre, que no em fa mal."},
{ "Spanish", "Puedo comer vidrio, no me hace daño."},
{ "Aragonés", "Puedo minchar beire, no me'n fa mal ."},
{ "Aranés", "(NEEDED)"},
{ "Mallorquín", "(NEEDED)"},
{ "Galician", "Eu podo xantar cristais e non cortarme."},
{ "European Portuguese", "Posso comer vidro, não me faz mal."},
{ "Brazilian Portuguese (8)", "Posso comer vidro, não me machuca."},
{ "Caboverdiano/Kabuverdianu (Cape Verde)", "M' podê cumê vidru, ca ta maguâ-m'."},
{ "Papiamentu", "Ami por kome glas anto e no ta hasimi daño."},
{ "Italian", "Posso mangiare il vetro e non mi fa male."},
{ "Milanese", "Sôn bôn de magnà el véder, el me fa minga mal."},
{ "Roman", "Me posso magna' er vetro, e nun me fa male."},
{ "Napoletano", "M' pozz magna' o'vetr, e nun m' fa mal."},
{ "Venetian", "Mi posso magnare el vetro, no'l me fa mae."},
{ "Zeneise (Genovese)", "Pòsso mangiâ o veddro e o no me fà mâ."},
{ "Sicilian", "Puotsu mangiari u vitru, nun mi fa mali."},
{ "Campinadese (Sardinia)", "(NEEDED)"},
{ "Lugudorese (Sardinia)", "(NEEDED)"},
{ "Romansch (Grischun)", "Jau sai mangiar vaider, senza che quai fa donn a mai."},
{ "Romany / Tsigane", "(NEEDED)"},
{ "Romanian", "Pot să mănânc sticlă și ea nu mă rănește."},
{ "Esperanto", "Mi povas manĝi vitron, ĝi ne damaĝas min."},
{ "Pictish", "(NEEDED)"},
{ "Breton", "(NEEDED)"},
{ "Cornish", "Mý a yl dybry gwéder hag éf ny wra ow ankenya."},
{ "Welsh", "Dw i'n gallu bwyta gwydr, 'dyw e ddim yn gwneud dolur i mi."},
{ "Manx Gaelic", "Foddym gee glonney agh cha jean eh gortaghey mee."},
{ "Old Irish (Ogham)", "᚛᚛ᚉᚑᚅᚔᚉᚉᚔᚋ ᚔᚈᚔ ᚍᚂᚐᚅᚑ ᚅᚔᚋᚌᚓᚅᚐ᚜"},
{ "Old Irish (Latin)", "Con·iccim ithi nglano. Ním·géna."},
{ "Irish", "Is féidir liom gloinne a ithe. Ní dhéanann sí dochar ar bith dom."},
{ "Ulster Gaelic", "Ithim-sa gloine agus ní miste damh é."},
{ "Scottish Gaelic", "S urrainn dhomh gloinne ithe; cha ghoirtich i mi."},
{ "Anglo-Saxon (Runes)", "ᛁᚳ᛫ᛗᚨᚷ᛫ᚷᛚᚨᛋ᛫ᛖᚩᛏᚪᚾ᛫ᚩᚾᛞ᛫ᚻᛁᛏ᛫ᚾᛖ᛫ᚻᛖᚪᚱᛗᛁᚪᚧ᛫ᛗᛖ᛬"},
{ "Anglo-Saxon (Latin)", "Ic mæg glæs eotan ond hit ne hearmiað me."},
{ "Middle English", "Ich canne glas eten and hit hirtiþ me nouȝt."},
{ "English", "I can eat glass and it doesn't hurt me."},
{ "English (IPA)", "[aɪ kæn iːt glɑːs ænd ɪt dɐz nɒt hɜːt miː] (Received Pronunciation)"},
{ "English (Braille)", "⠊⠀⠉⠁⠝⠀⠑⠁⠞⠀⠛⠇⠁⠎⠎⠀⠁⠝⠙⠀⠊⠞⠀⠙⠕⠑⠎⠝⠞⠀⠓⠥⠗⠞⠀⠍⠑"},
{ "Jamaican", "Mi kian niam glas han i neba hot mi."},
{ "Lalland Scots / Doric", "Ah can eat gless, it disnae hurt us."},
{ "Glaswegian", "(NEEDED)"},
{ "Gothic (4)", "ЌЌЌ ЌЌЌЍ Ќ̈ЍЌЌ, ЌЌ ЌЌЍ ЍЌ ЌЌЌЌ ЌЍЌЌЌЌЌ."},
{ "Old Norse (Runes)", "ᛖᚴ ᚷᛖᛏ ᛖᛏᛁ ᚧ ᚷᛚᛖᚱ ᛘᚾ ᚦᛖᛋᛋ ᚨᚧ ᚡᛖ ᚱᚧᚨ ᛋᚨᚱ"},
{ "Old Norse (Latin)", "Ek get etið gler án þess að verða sár."},
{ "Norsk / Norwegian (Nynorsk)", "Eg kan eta glas utan å skada meg."},
{ "Norsk / Norwegian (Bokmål)", "Jeg kan spise glass uten å skade meg."},
{ "Føroyskt / Faroese", "Eg kann eta glas, skaðaleysur."},
{ "Íslenska / Icelandic", "Ég get etið gler án þess að meiða mig."},
{ "Svenska / Swedish", "Jag kan äta glas utan att skada mig."},
{ "Dansk / Danish", "Jeg kan spise glas, det gør ikke ondt på mig."},
{ "Sønderjysk", "Æ ka æe glass uhen at det go mæ naue."},
{ "Frysk / Frisian", "Ik kin glês ite, it docht me net sear."},
{ "Nederlands / Dutch", "Ik kan glas eten, het doet mĳ geen kwaad."},
{ "Kirchröadsj/Bôchesserplat", "Iech ken glaas èèse, mer 't deet miech jing pieng."},
{ "Afrikaans", "Ek kan glas eet, maar dit doen my nie skade nie."},
{ "Lëtzebuergescht / Luxemburgish", "Ech kan Glas iessen, daat deet mir nët wei."},
{ "Deutsch / German", "Ich kann Glas essen, ohne mir zu schaden."},
{ "Ruhrdeutsch", "Ich kann Glas verkasematuckeln, ohne dattet mich wat jucken tut."},
{ "Langenfelder Platt", "Isch kann Jlaas kimmeln, uuhne datt mich datt weh dääd."},
{ "Lausitzer Mundart (Lusatian)", "Ich koann Gloos assn und doas dudd merr ni wii."},
{ "Odenwälderisch", "Iech konn glaasch voschbachteln ohne dass es mir ebbs daun doun dud."},
{ "Sächsisch / Saxon", "'sch kann Glos essn, ohne dass'sch mer wehtue."},
{ "Pfälzisch", "Isch konn Glass fresse ohne dasses mer ebbes ausmache dud."},
{ "Schwäbisch / Swabian", "I kå Glas frässa, ond des macht mr nix!"},
{ "Deutsch (Voralberg)", "I ka glas eassa, ohne dass mar weh tuat."},
{ "Bayrisch / Bavarian", "I koh Glos esa, und es duard ma ned wei."},
{ "Allemannisch", "I kaun Gloos essen, es tuat ma ned weh."},
{ "Schwyzerdütsch (Zürich)", "Ich chan Glaas ässe, das schadt mir nöd."},
{ "Schwyzerdütsch (Luzern)", "Ech cha Glâs ässe, das schadt mer ned."},
{ "Plautdietsch", "(NEEDED)"},
{ "Hungarian", "Meg tudom enni az üveget, nem lesz tőle bajom."},
{ "Suomi / Finnish", "Voin syödä lasia, se ei vahingoita minua."},
{ "Sami (Northern)", "Sáhtán borrat lása, dat ii leat bávččas."},
{ "Erzian", "Мон ярсан суликадо, ды зыян эйстэнзэ а ули."},
{ "Northern Karelian", "Mie voin syvvä lasie ta minla ei ole kipie."},
{ "Southern Karelian", "Minä voin syvvä st'oklua dai minule ei ole kibie."},
{ "Vepsian", "(NEEDED)"},
{ "Votian", "(NEEDED)"},
{ "Livonian", "(NEEDED)"},
{ "Estonian", "Ma võin klaasi süüa, see ei tee mulle midagi."},
{ "Latvian", "Es varu ēst stiklu, tas man nekaitē."},
{ "Lithuanian", "Aš galiu valgyti stiklą ir jis manęs nežeidžia"},
{ "Old Prussian", "(NEEDED)"},
{ "Sorbian (Wendish)", "(NEEDED)"},
{ "Czech", "Mohu jíst sklo, neublíží mi."},
{ "Slovak", "Môžem jesť sklo. Nezraní ma."},
{ "Polska / Polish", "Mogę jeść szkło i mi nie szkodzi."},
{ "Slovenian", "Lahko jem steklo, ne da bi mi škodovalo."},
{ "Bosnian, Croatian, Montenegrin and Serbian (Latin)", "Ja mogu jesti staklo, i to mi ne šteti."},
{ "Bosnian, Montenegrin and Serbian (Cyrillic)", "Ја могу јести стакло, и то ми не штети."},
{ "Macedonian", "Можам да јадам стакло, а не ме штета."},
{ "Russian", "Я могу есть стекло, оно мне не вредит."},
{ "Belarusian (Cyrillic)", "Я магу есці шкло, яно мне не шкодзіць."},
{ "Belarusian (Lacinka)", "Ja mahu jeści škło, jano mne ne škodzić."},
{ "Ukrainian", "Я можу їсти скло, і воно мені не зашкодить."},
{ "Bulgarian", "Мога да ям стъкло, то не ми вреди."},
{ "Georgian", "მინას ვჭამ და არა მტკივა."},
{ "Armenian", "Կրնամ ապակի ուտել և ինծի անհանգիստ չըներ։"},
{ "Albanian", "Unë mund të ha qelq dhe nuk më gjen gjë."},
{ "Turkish", "Cam yiyebilirim, bana zararı dokunmaz."},
{ "Turkish (Ottoman)", "جام ييه بلورم بڭا ضررى طوقونمز"},
{ "Bangla / Bengali", "আমি কাঁচ খেতে পারি, তাতে আমার কোনো ক্ষতি হয় না।"},
{ "Marathi", "मी काच खाऊ शकतो, मला ते दुखत नाही."},
{ "Kannada", "ನನಗೆ ಹಾನಿ ಆಗದೆ, ನಾನು ಗಜನ್ನು ತಿನಬಹುದು"},
{ "Hindi", "मैं काँच खा सकता हूँ और मुझे उससे कोई चोट नहीं पहुंचती."},
{ "Tamil", "நான் கண்ணாடி சாப்பிடுவேன், அதனால் எனக்கு ஒரு கேடும் வராது."},
{ "Telugu", "నేను గాజు తినగలను మరియు అలా చేసినా నాకు ఏమి ఇబ్బంది లేదు"},
{ "Sinhalese", "මට වීදුරු කෑමට හැකියි. එයින් මට කිසි හානියක් සිදු නොවේ."},
{ "Urdu(3)", "میں کانچ کھا سکتا ہوں اور مجھے تکلیف نہیں ہوتی ۔"},
{ "Pashto(3)", "زه شيشه خوړلې شم، هغه ما نه خوږوي"},
{ "Farsi / Persian(3)", "من می توانم بدونِ احساس درد شيشه بخورم"},
{ "Arabic(3)", "أنا قادر على أكل الزجاج و هذا لا يؤلمني."},
{ "Aramaic", "(NEEDED)"},
{ "Maltese", "Nista' niekol il-ħġieġ u ma jagħmilli xejn."},
{ "Hebrew(3)", "אני יכול לאכול זכוכית וזה לא מזיק לי."},
{ "Yiddish(3)", "איך קען עסן גלאָז און עס טוט מיר נישט װײ."},
{ "Judeo-Arabic", "(NEEDED)"},
{ "Ladino", "(NEEDED)"},
{ "Gǝʼǝz", "(NEEDED)"},
{ "Amharic", "(NEEDED)"},
{ "Twi", "Metumi awe tumpan, ɜnyɜ me hwee."},
{ "Hausa (Latin)", "Inā iya taunar gilāshi kuma in gamā lāfiyā."},
{ "Hausa (Ajami) (2)", "إِنا إِىَ تَونَر غِلَاشِ كُمَ إِن غَمَا لَافِىَا"},
{ "Yoruba(4)", "Mo lè je̩ dígí, kò ní pa mí lára."},
{ "Lingala", "Nakokí kolíya biténi bya milungi, ekosála ngáí mabé tɛ́."},
{ "(Ki)Swahili", "Naweza kula bilauri na sikunyui."},
{ "Malay", "Saya boleh makan kaca dan ia tidak mencederakan saya."},
{ "Tagalog", "Kaya kong kumain nang bubog at hindi ako masaktan."},
{ "Chamorro", "Siña yo' chumocho krestat, ti ha na'lalamen yo'."},
{ "Fijian", "Au rawa ni kana iloilo, ia au sega ni vakacacani kina."},
{ "Javanese", "Aku isa mangan beling tanpa lara."},
{ "Burmese", "က္ယ္ဝန္‌တော္‌၊က္ယ္ဝန္‌မ မ္ယက္‌စားနုိင္‌သည္‌။ ၎က္ရောင္‌့ ထိခုိက္‌မ္ဟု မရ္ဟိပာ။ (9)"},
{ "Vietnamese (quốc ngữ)", "Tôi có thể ăn thủy tinh mà không hại gì."},
{ "Vietnamese (nôm) (4)", "些 ࣎ 世 咹 水 晶 ও 空 ࣎ 害 咦"},
{ "Khmer", "ខ្ញុំអាចញុំកញ្ចក់បាន ដោយគ្មានបញ្ហារ"},
{ "Lao", "ຂອ້ຍກິນແກ້ວໄດ້ໂດຍທີ່ມັນບໍ່ໄດ້ເຮັດໃຫ້ຂອ້ຍເຈັບ."},
{ "Thai", "ฉันกินกระจกได้ แต่มันไม่ทำให้ฉันเจ็บ"},
{ "Mongolian (Cyrillic)", "Би шил идэй чадна, надад хортой биш"},
{ "Mongolian (Classic) (5)", "ᠪᠢ ᠰᠢᠯᠢ ᠢᠳᠡᠶᠦ ᠴᠢᠳᠠᠨᠠ ᠂ ᠨᠠᠳᠤᠷ ᠬᠣᠤᠷᠠᠳᠠᠢ ᠪᠢᠰᠢ"},
{ "Dzongkha", "(NEEDED)"},
{ "Nepali", "﻿म काँच खान सक्छू र मलाई केहि नी हुन्‍न् ।"},
{ "Tibetan", "ཤེལ་སྒོ་ཟ་ནས་ང་ན་གི་མ་རེད།"},
{ "Chinese", "我能吞下玻璃而不伤身体。"},
{ "Chinese (Traditional)", "我能吞下玻璃而不傷身體。"},
{ "Taiwanese(6)", "Góa ē-tàng chia̍h po-lê, mā bē tio̍h-siong."},
{ "Japanese", "私はガラスを食べられます。それは私を傷つけません。"},
{ "Korean", "나는 유리를 먹을 수 있어요. 그래도 아프지 않아요"},
{ "Bislama", "Mi save kakae glas, hemi no save katem mi."},
{ "Hawaiian", "Hiki iaʻu ke ʻai i ke aniani; ʻaʻole nō lā au e ʻeha."},
{ "Marquesan", "E koʻana e kai i te karahi, mea ʻā, ʻaʻe hauhau."},
{ "Inuktitut (10)", "ᐊᓕᒍᖅ ᓂᕆᔭᕌᖓᒃᑯ ᓱᕋᙱᑦᑐᓐᓇᖅᑐᖓ"},
{ "Chinook Jargon", "Naika məkmək kakshət labutay, pi weyk ukuk munk-sik nay."},
{ "Navajo", "Tsésǫʼ yishą́ągo bííníshghah dóó doo shił neezgai da."},
{ "Cherokee (and Cree, Chickasaw, Cree, Micmac, Ojibwa, Lakota, Náhuatl, Quechua, Aymara, and other American languages)", "(NEEDED)"},
{ "Garifuna", "(NEEDED)"},
{ "Gullah", "(NEEDED)"},
{ "Lojban", "mi kakne le nu citka le blaci. iku'i le se go'i na xrani mi"},
{ "Nórdicg", "Ljœr ye caudran créneþ ý jor cẃran."},
    };


    std::vector<uint32_t> scalars;

    for(const std::pair<std::string, std::string>& data : samples)
    {
        const std::string& inputStr = data.second;
        std::cout << "-> original: " <<  inputStr << std::endl;
        
        scalars.clear();
        if (encode_utf8_to_utf32(inputStr.c_str(), scalars) == false)
        {
            return -1;
        }

        for (const uint32_t codePoint : scalars)
        {
            if (codePoint == 0) {
                break;
            }
            //printf("U+%04X ", codePoint);
        }
        //printf("\n");
        std::string temp;
        encode_utf32_to_utf8(scalars, temp);
        std::cout <<"-> decoded:  " << temp << std::endl;
        std::cout << std::endl;
    }

    {
        const std::string str="Argélia";
        std::cout << "peeking from: " << str << std::endl;

        const char* charPtr = str.c_str();
        size_t pos = 0;
        uint32_t utf32;
        size_t numBytes = 0;
        while((utf32 = detail::utf8_to_utf32(reinterpret_cast<const uint8_t*>(charPtr), &numBytes)) != uint32_t(-1))
        {
            std::string utf8; utf8.resize(4);
            const size_t numBytes = detail::utf32_to_utf8(utf32, &utf8[0], utf8.size());
            std::cout << "pos: " << pos
                << " u32(#"<< numBytes <<"): " << utf32
                << " -> utf8: " << utf8 << std::endl;
        }
    }

    return 0;
}
