#include <tut/tut.hpp>
#include <sstream>
#include <algorithm>
#include <locale>
#include <boost/locale.hpp>
#include <boost/algorithm/string.hpp>
#include <boost/iostreams/stream.hpp>
#include <boost/iostreams/device/array.hpp>

#include "../../src/SaxParser.h"

namespace {
  struct setup {
    xml::sax::Parser parser;
  };
}

namespace tut {
  typedef test_group<setup> tg;
  tg saxparser_test_group("sax parser");

  typedef tg::object testobject;

  template<>
  template<>
  void testobject::test<1>()
  {
    set_test_name("well formed");
    std::istringstream ss("<root></root>  ");
    parser.parse(ss);

    ss.str("<root>");
    ss.clear();
    try {
      parser.parse(ss);
    }
    catch (const std::system_error &ex) {
      ensure_equals(ex.code().value(), xml::PREMATURE_EOF);
    }

    ss.str("<root></ruut>");
    ss.clear();
    try {
      parser.parse(ss);
    }
    catch (const std::system_error &ex) {
      ensure_equals(ex.code().value(), xml::TAG_MISMATCH);
    }

    ss.str("<root><0sub></0sub></root>");
    ss.clear();
    try {
      parser.parse(ss);
    }
    catch (const std::system_error &ex) {
      ensure_equals(ex.code().value(), xml::MALFORMED);
    }

  }

  template<>
  template<>
  void testobject::test<2>()
  {
    set_test_name("well formed with text");
    std::istringstream ss("<root><sub>text</sub></root>");
    parser.parse(ss);
  }

  template<>
  template<>
  void testobject::test<3>()
  {
    set_test_name("well formed with text and attributes");
    std::istringstream ss("<root naosei='20'><sub attr=\"10\" other='dsfsdfs'>text</sub></root>");
    parser.parse(ss);
  }

  template<>
  template<>
  void testobject::test<4>()
  {
    set_test_name("well formed with empty tags");
    std::istringstream ss("<root naosei='20'><sub attr=\"10\" other='dsfsdfs' /></root>");
    parser.parse(ss);
    ss.str("<root><sub/></root>");
    ss.clear();
    parser.parse(ss);
  }

  template<>
  template<>
  void testobject::test<5>()
  {
    using namespace xml::sax;
    set_test_name("events start and end");
    std::istringstream ss("<root naosei='20'>\n<sub attr=\"10\" other='dsfsdfs' />    <tag2   >texto</tag2>   </root>");
    Parser::TagType startDoc;
    Parser::TagType tags, tagsEnd;
    parser.startDocument([&](Parser::TagType const & name, AttributeIterator & it) { startDoc = name; });
    parser.endDocument([&](Parser::TagType const & name) { ensure_equals(name, startDoc); });
    parser.startTag([&](Parser::TagType const & name, AttributeIterator & it) { tags += name; });
    parser.endTag([&](Parser::TagType const & name) { tagsEnd += name; });
    parser.parse(ss);
    ensure_equals(startDoc, "root");
    ensure_equals(tags, "subtag2");
    ensure_equals(tagsEnd, tags);
  }

  template<>
  template<>
  void testobject::test<6>()
  {
    using namespace xml::sax;
    set_test_name("event start using attributes");
    std::istringstream ss(
      "<root naosei='20'>\n"
      "<sub attr=\"10\" cost='BBR' other='dsfsdfs' />    "
      "<tag2   >texto</tag2>   "
      "</root    >");
    parser.startTag([](Parser::TagType const & name, AttributeIterator & it) {
      if (name == "sub") {
        int found = 0;
        auto attribute = it.getNext();
        while (attribute && found < 2) {
          if ((attribute->name == "attr" && attribute->value == "10") ||
            (attribute->name == "cost" && attribute->value == "BBR")) ++found;
          attribute = it.getNext();
        }
        if (found != 2) throw xml::ABORTED;
      }
    });
    parser.parse(ss);
  }

  template<>
  template<>
  void testobject::test<7>()
  {
    using namespace xml::sax;
    set_test_name("character data");
    std::istringstream ss(
      "<root naosei='20'>\n"
      "<tag1>texto inicial</tag1>"
      "<sub attr=\"10\" cost='BBR' other='dsfsdfs' />    "
      "<tag2   >texto</tag2>   "
      "</root    >");
    std::string texto;
    parser.characters([&](CharIterator & it) {
      texto += it.getText();
    });
    parser.parse(ss);
    ensure_equals(texto, "texto inicialtexto");
  }

  template<>
  template<>
  void testobject::test<8>()
  {
    using namespace xml::sax;
    set_test_name("processing instructions");
    std::istringstream ss(
      "<?xml encoding=\"ISO-8859-1\"?>"
      "<root naosei='20'>\n"
      "<tag1>texto inicial</tag1>"
      "<?teucu instrucao?>"
      "<sub attr=\"10\" cost='BBR' other='dsfsdfs' />    "
      "<tag2   >texto</tag2>   "
      "</root    >");
    std::string pi;
    std::string args;
    parser.processingInstruction([&](Parser::TagType const & name, std::string const & arguments) {
      pi += name;
      args += arguments;
    });
    parser.parse(ss);
    ensure_equals(pi, "xmlteucu");
    ensure_equals(args, "encoding=\"ISO-8859-1\"instrucao");
  }

  template<>
  template<>
  void testobject::test<9>()
  {
    set_test_name("well formed with prologue");
    std::istringstream ss(
      "<?xml version=\"1.0\" encoding=\"ISO-8859-1\"?>\n\r"
      "<root><sub></sub></root>");
    parser.parse(ss);

    ss.str("<root><sub></sub><?xml version=\"1.0\" encoding=\"ISO-8859-1\"?></root>");
    ss.clear();
    try {
      parser.parse(ss);
    }
    catch (const std::system_error &ex) {
      ensure_equals(ex.code().value(), xml::MALFORMED);
    }
  }

  template<>
  template<>
  void testobject::test<10>()
  {
    set_test_name("comments");
    std::istringstream ss("<root><sub a='1'>alasksf</sub><!-- this game sucks --><a>dd</a></root>");
    parser.parse(ss);
  }

  template<>
  template<>
  void testobject::test<11>()
  {
    using namespace xml::sax;
    set_test_name("elements");
    std::istringstream ss(
      "<?xml encoding=\"ISO-8859-1\"?>"
      "<!DOCTYPE greeting SYSTEM \"hello.dtd\">"
      "<root naosei='20'>\n"
      "<tag1>texto inicial</tag1>"
      "<!ELEMENT br EMPTY>"
      "<sub attr=\"10\" cost='BBR' other='dsfsdfs' />    "
      "<tag2   >texto</tag2>   "
      "<!NOTATION usdruvs PUBLIC argh>"
      "</root    >");
    std::string pi;
    std::string args;
    parser.element([&](Parser::TagType const & name, std::string const & arguments) {
      pi += name;
      args += arguments;
    });
    parser.parse(ss);
    ensure_equals(pi, "DOCTYPE""ELEMENT""NOTATION");
    ensure_equals(args, "greeting SYSTEM \"hello.dtd\"""br EMPTY""usdruvs PUBLIC argh");
  }

  template<>
  template<>
  void testobject::test<12>()
  {
    using namespace xml::sax;
    set_test_name("cdata 1");
    std::istringstream ss(
      "<?xml encoding=\"ISO-8859-1\"?>"
      "<!DOCTYPE greeting SYSTEM \"hello.dtd\">"
      "<root naosei='20'>\n"
      "<tag1>antes</tag1>"
      "<tagsafada><![CDATA[123456]]></tagsafada>"
      "<!ELEMENT br EMPTY>"
      "<tag2   >meio</tag2>   "
      "<tagsafada2><![CDATA[ ai [dede] ]]></tagsafada2>"
      "<tag3   >depois</tag3>   "
      "</root    >");
    std::string texto;
    parser.characters([&](CharIterator & it) {
      texto += it.getText();
    });
    parser.parse(ss);
    ensure_equals(texto, "antes""123456""meio"" ai [dede] ""depois");
  }

  template<>
  template<>
  void testobject::test<13>()
  {
    using namespace xml::sax;
    set_test_name("cdata 2");
    std::istringstream ss(
      "<root naosei='20'>\n"
      "<tagsafada3><![CDATA[ai [[didi]]]]></tagsafada3>"
      "<tag3   >depois</tag3>   "
      "</root    >");
    std::string texto;
    parser.characters([&](CharIterator & it) {
      texto += it.getText();
    });
    parser.parse(ss);
    ensure_equals(texto, "ai [[didi]]""depois");
  }

  template<>
  template<>
  void testobject::test<14>()
  {
    set_test_name("not aborted");
    std::istringstream ss("<root></root>  ");
    ensure_not(parser.parse(ss));
  }

  template<>
  template<>
  void testobject::test<15>()
  {
    using namespace xml::sax;
    set_test_name("abort and continue");
    std::istringstream ss(
      "<root naosei='20'>\n"
      "<tag1>aah</tag1>"
      "<tagdef tagName='superTag' other='dsfsdfs' another='xxx' />    "
      "<otherTag>irrelevant text</otherTag>   "
      "<superTag>this is the answer</superTag>"
      "<tag2>bah</tag2>"
      "</root>");
    std::string tagName;
    parser.startTag([&](Parser::TagType const & name, AttributeIterator & it) {
      if (name == "tagdef") {
        for (auto attribute = it.getNext(); attribute; attribute = it.getNext()) {
          if (attribute->name == "tagName") {
            tagName = attribute->value;
            throw xml::ABORTED;
          }
        }
      }
    });

    if (parser.parse(ss)) {
      bool getText = false;
      std::string texto;
      parser.startTag([&](Parser::TagType const & name, AttributeIterator & it) {
        if (name == tagName) {
          getText = true;
        }
      });
      parser.endTag([&](Parser::TagType const & name) {
        if (name == tagName) throw xml::ABORTED;
      });
      parser.characters([&](CharIterator & it) {
        if (getText) {
          texto += it.getText();
        }
      });
      if (parser.parseContinue(ss)) {
        ensure_equals(texto, "this is the answer");
      }
      else {
        fail("parsing was not aborted on second time");
      }
    }
    else {
      fail("parsing was not aborted on first time");
    }
  }

  template<>
  template<>
  void testobject::test<16>()
  {
    using namespace xml::sax;
    set_test_name("abort and continue");
    std::istringstream ss(
      "<root>"
      "<fieldtag>aah1</fieldtag>"
      "<fieldtag>aah2</fieldtag>"
      "</root>");
    parser.endTag([&](Parser::TagType const & name) {
      if (name == "fieldtag") throw xml::ABORTED;
    });

    ensure_equals(parser.parse(ss), true);
    ensure_equals(parser.parseContinue(ss), true);
    ensure_equals(parser.parseContinue(ss), false);
  }

  static std::locale locale;
  // trim from start (in place)
  static inline std::string &ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](int ch) {
      return !std::isspace(ch, locale);
    }));
    return s;
  }

  // trim from end (in place)
  static inline std::string &rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](int ch) {
      return !std::isspace(ch, locale);
    }).base(), s.end());
    return s;
  }

  // trim from both ends (in place)
  static inline std::string &trim(std::string &s) {
    ltrim(s);
    rtrim(s);
    return s;
  }
  template<>
  template<>
  void testobject::test<17>()
  {
    using namespace xml::sax;
    set_test_name("cdata 1");
    char ss1[] = (R"V0G0N(<?xml version="1.0" encoding="utf-8"?>
<d:multistatus xmlns:d="DAV:">
  <d:response>
    <d:href>
      /
    </d:href>
    <d:propstat>
      <d:status>
        HTTP/1.1 200 OK
      </d:status>
      <d:prop>
        <d:creationdate>
          1970-01-01T00:00:00Z
        </d:creationdate>
        <d:displayname>
          disk
        </d:displayname>
        <d:getcontentlength>
          0
        </d:getcontentlength>
        <d:getlastmodified>
          Thu, 01 Jan 1970 00:00:00 GMT
        </d:getlastmodified>
        <d:resourcetype>
          <d:collection/>
        </d:resourcetype>
      </d:prop>
    </d:propstat>
  </d:response>
  <d:response>
    <d:href>
      /Documents/
    </d:href>
    <d:propstat>
      <d:status>
        HTTP/1.1 200 OK
      </d:status>
      <d:prop>
        <d:creationdate>
          2012-03-24T09:00:43Z
        </d:creationdate>
        <d:displayname>
          Documents
        </d:displayname>
        <d:getcontentlength>
          0
        </d:getcontentlength>
        <d:getlastmodified>
          Sat, 24 Mar 2012 09:00:43 GMT
        </d:getlastmodified>
        <d:resourcetype>
          <d:collection/>
        </d:resourcetype>
      </d:prop>
    </d:propstat>
  </d:response>
  <d:response>
    <d:href>
      /readme.pdf
    </d:href>
    <d:propstat>
      <d:status>
        HTTP/1.1 200 OK
      </d:status>
      <d:prop>
        <d:creationdate>
          2012-04-09T10:56:13Z
        </d:creationdate>
        <d:displayname>
          readme.pdf
        </d:displayname>
        <d:getcontentlength>
          455833
        </d:getcontentlength>
        <d:getcontenttype>
          application/pdf
        </d:getcontenttype>
        <d:getlastmodified>
          Mon, 09 Apr 2012 10:56:13 GMT
        </d:getlastmodified>
        <d:resourcetype/>
      </d:prop>
    </d:propstat>
  </d:response>
</d:multistatus>)V0G0N");
    std::string uri = "/";
    enum
    {
      document,
      response,
      propstat,
      resourcetype
    } nesting = document;
    enum
    {
      ignore,
      href,
      status,
      displayname,
      getcontentlength,
      getlastmodified
    } current_state = ignore;
    bool in_response = false, in_propstat = false;
    struct
    {
      int status;
      std::string href;
      std::wstring name;
      std::string modified;
      long long size;
      std::string resourcetype;
    } item;

    parser.processingInstruction([&](const std::string &pi, const std::string &args)
    {
      auto range = boost::ifind_first(args, "encoding");
      if (range)
      {
        auto iterator = range.end();
        while (iterator != args.end() && *iterator != '=')
          ++iterator;
        while (iterator != args.end() && std::isspace(*(++iterator), locale));
        if (iterator == args.end())
          return;
        else
        {
          std::string encoding;
          char cquote = 0;
          if (*iterator == '\'' || *iterator == '"')
            cquote = *iterator;
          while (iterator != args.end())
          {
            if (cquote == 0 && std::isspace(*(++iterator), locale))
              break;
            else if (cquote && *(++iterator) == cquote)
              break;
            else
              encoding.push_back(*iterator);
          }
          if (encoding.size())
            locale = boost::locale::generator::generator().generate(encoding);
        }
      }
    });
    parser.characters([&](CharIterator & it) {
      switch (current_state)
      {
      case href:
        item.href = trim(it.getText());
        break;
      case status:
        item.status = atoi(trim(it.getText()).c_str() + 9);
        break;
      case displayname:
        if (item.href == uri)
        {
          item.name = L".";
        }
        else
        {
          std::string narrow = it.getText();
          trim(narrow);
          if (narrow.size())
          {
            item.name.resize(narrow.size());

            mbstate_t mystate;
            const std::codecvt<char, wchar_t, mbstate_t>& myfacet =
              std::use_facet<std::codecvt<char, wchar_t, mbstate_t> >(std::locale());

            const char *cw;
            wchar_t *wcw;

            if (myfacet.out(mystate, narrow.c_str(), narrow.c_str() + narrow.size(), cw,
              item.name.data(), item.name.data() + item.name.size(), wcw) !=
              std::codecvt<wchar_t, char, mbstate_t>::ok)
            {
              throw std::runtime_error("character translation failed");
            }
            item.name.resize(wcw - item.name.data());
          }
        }
        break;
      case getlastmodified:
        item.modified = trim(it.getText());
        break;
      case getcontentlength:
        item.size = atoll(trim(it.getText()).c_str());
        break;
      default:
        //ignore
        break;
      }
    });

    parser.startTag([&](const std::string &name, xml::sax::AttributeIterator &it)
    {
      size_t ndx = name.find(':');
      if (ndx == std::string::npos)
        ndx = 0;
      else
        ndx++;
      switch (name[ndx++])
      {
      case 'c':
      case 'C': //collection
        if (nesting == resourcetype && _stricmp(name.c_str() + ndx, "ollection") == 0)
        {
          item.resourcetype = "collection";
        }
        break;
      case 'd':
      case 'D': //displayname
        if (nesting == propstat && _stricmp(name.c_str() + ndx, "isplayname") == 0)
        {
          current_state = displayname;
        }
        break;
      case 'g':
      case 'G': //getcontentlength or getlastmodified
        if (nesting == propstat && tolower(name[ndx++]) == 'e' && tolower(name[ndx++]) == 't')
        {
          switch (name[ndx++])
          {
          case 'c':
          case 'C': //getcontentlength
            if (_stricmp(name.c_str() + ndx, "ontentlength") == 0)
            {
              current_state = getcontentlength;
            }
            break;
          case 'l':
          case 'L': //getlastmodified
            if (_stricmp(name.c_str() + ndx, "astmodified") == 0)
            {
              current_state = getlastmodified;
            }
            break;
          }
        }
        break;
      case 'h':
      case 'H': //href
        if (nesting == response && _stricmp(name.c_str() + ndx, "ref") == 0)
        {
          current_state = href;
        }
        break;
      case 'p':
      case 'P': //propstat
        if (nesting == response && _stricmp(name.c_str() + ndx, "ropstat") == 0)
        {
          nesting = propstat;
        }
        break;
      case 'r':  //response or resource
      case 'R':
        if (tolower(name[ndx++]) == 'e' && tolower(name[ndx++]) == 's')
        {
          switch (name[ndx++])
          {
          case 'p':
          case 'P': //response
            if (_stricmp(name.c_str() + ndx, "onse") == 0)
            {
              item.href.clear();
              item.status = 0;
              item.size = -1;
              item.modified.clear();
              item.name.clear();
              item.resourcetype = "unknown";
              nesting = response;
            }
            break;
          case 'o':
          case 'O': //resourcetype
            if (nesting == propstat && _stricmp(name.c_str() + ndx, "urcetype") == 0)
            {
              nesting = resourcetype;
              item.resourcetype = "file";
            }
            break;
          }
        }
        break;
      case 's':
      case 'S': //status
        if (nesting == propstat && _stricmp(name.c_str() + ndx, "tatus") == 0)
        {
          current_state = status;
        }
        break;
      }
    });

    parser.endTag([&](const std::string &name)
    {
      current_state = ignore;
      size_t ndx = name.find(':');
      if (ndx == std::string::npos)
        ndx = 0;
      else
        ndx++;
      switch (name[ndx++])
      {
      case 'p':
      case 'P': //propstat
        if (nesting == propstat && _stricmp(name.c_str() + ndx, "ropstat") == 0)
        {
          nesting = response;
        }
        break;
      case 'r':  //response or resource
      case 'R':
        if (tolower(name[ndx++]) == 'e' && tolower(name[ndx++]) == 's')
        {
          switch (name[ndx++])
          {
          case 'p':
          case 'P': //response
            if (nesting == response && _stricmp(name.c_str() + ndx, "onse") == 0)
            {
              nesting = document;
              if (item.status == 200)
              {
                std::wcout << item.name;
                std::cout << "\ttype = " << item.resourcetype;
                if (item.size >= 0)
                  std::cout << ", size = " << item.size;
                if (item.modified.size())
                  std::cout << ", modifed " << item.modified;
                std::cout << std::endl;
              }
            }
            break;
          case 'o':
          case 'O': //resourcetype
            if (nesting == resourcetype && _stricmp(name.c_str() + ndx, "urcetype") == 0)
            {
              nesting = propstat;
            }
            break;
          }
        }
        break;
      }
    });

    boost::iostreams::array_source array1(ss1, _countof(ss1) - 1);
    boost::iostreams::stream<boost::iostreams::array_source> source1(array1);
    parser.parse(source1);
  }
}
