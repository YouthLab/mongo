/* Copyright 2013 10gen Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <map>
#include <sstream>

#include "mongo/bson/util/builder.h"
#include "mongo/unittest/unittest.h"
#include "mongo/util/options_parser/environment.h"
#include "mongo/util/options_parser/option_description.h"
#include "mongo/util/options_parser/option_section.h"
#include "mongo/util/options_parser/options_parser.h"

namespace {

    using mongo::ErrorCodes;
    using mongo::Status;

    namespace moe = mongo::optionenvironment;

#define TEST_CONFIG_PATH(x) "src/mongo/util/options_parser/test_config_files/" x

    class OptionsParserTester : public moe::OptionsParser {
    public:
        Status readConfigFile(const std::string& filename, std::string* config) {
            if (filename != _filename) {
                ::mongo::StringBuilder sb;
                sb << "Parser using filename: " << filename <<
                      " which does not match expected filename: " << _filename;
                return Status(ErrorCodes::InternalError, sb.str());
            }
            *config = _config;
            return Status::OK();
        }
        void setConfig(const std::string& filename, const std::string& config) {
            _filename = filename;
            _config = config;
        }
    private:
        std::string _filename;
        std::string _config;
    };

    TEST(Registration, DuplicateSingleName) {
        moe::OptionSection testOpts;
        ASSERT_OK(testOpts.addOption(moe::OptionDescription("dup", "dup", moe::Switch, "dup")));
        ASSERT_NOT_OK(testOpts.addOption(moe::OptionDescription("new", "dup", moe::Switch, "dup")));
    }

    TEST(Registration, DuplicateDottedName) {
        moe::OptionSection testOpts;
        ASSERT_OK(testOpts.addOption(moe::OptionDescription("dup", "dup", moe::Switch, "dup")));
        ASSERT_NOT_OK(testOpts.addOption(moe::OptionDescription("dup", "new", moe::Switch, "dup")));
    }

    TEST(Registration, DuplicatePositional) {
        moe::OptionSection testOpts;
        ASSERT_OK(testOpts.addPositionalOption(moe::PositionalOptionDescription("positional",
                                                                                moe::Int)));
        ASSERT_NOT_OK(testOpts.addPositionalOption(moe::PositionalOptionDescription("positional",
                                                                                    moe::Int)));
    }

    TEST(Parsing, Good) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("help", "help", moe::Switch, "Display help"));
        testOpts.addOption(moe::OptionDescription("port", "port", moe::Int, "Port"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--port");
        argv.push_back("5");
        argv.push_back("--help");
        std::map<std::string, std::string> env_map;

        moe::Value value;
        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        ASSERT_OK(environment.get(moe::Key("help"), &value));
        ASSERT_OK(environment.get(moe::Key("port"), &value));
        int port;
        ASSERT_OK(value.get(&port));
        ASSERT_EQUALS(port, 5);
    }

    TEST(Parsing, SubSection) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        moe::OptionSection subSection("Section Name");

        subSection.addOption(moe::OptionDescription("port", "port", moe::Int, "Port"));
        testOpts.addSection(subSection);

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--port");
        argv.push_back("5");
        std::map<std::string, std::string> env_map;

        moe::Value value;
        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        ASSERT_OK(environment.get(moe::Key("port"), &value));
        int port;
        ASSERT_OK(value.get(&port));
        ASSERT_EQUALS(port, 5);
    }

    TEST(Parsing, StringVector) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("multival", "multival", moe::StringVector,
                                                  "Multiple Values"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--multival");
        argv.push_back("val1");
        argv.push_back("--multival");
        argv.push_back("val2");
        std::map<std::string, std::string> env_map;

        moe::Value value;
        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        ASSERT_OK(environment.get(moe::Key("multival"), &value));
        std::vector<std::string> multival;
        std::vector<std::string>::iterator multivalit;
        ASSERT_OK(value.get(&multival));
        multivalit = multival.begin();
        ASSERT_EQUALS(*multivalit, "val1");
        multivalit++;
        ASSERT_EQUALS(*multivalit, "val2");
    }

    TEST(Parsing, Positional) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addPositionalOption(moe::PositionalOptionDescription("positional", moe::String));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("positional");
        std::map<std::string, std::string> env_map;

        moe::Value value;
        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        ASSERT_OK(environment.get(moe::Key("positional"), &value));
        std::string positional;
        ASSERT_OK(value.get(&positional));
        ASSERT_EQUALS(positional, "positional");
    }

    TEST(Parsing, PositionalTooMany) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addPositionalOption(moe::PositionalOptionDescription("positional", moe::String));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("positional");
        argv.push_back("extrapositional");
        std::map<std::string, std::string> env_map;

        moe::Value value;
        ASSERT_NOT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

    TEST(Parsing, PositionalAndFlag) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addPositionalOption(moe::PositionalOptionDescription("positional", moe::String));
        testOpts.addOption(moe::OptionDescription("port", "port", moe::Int, "Port"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("positional");
        argv.push_back("--port");
        argv.push_back("5");
        std::map<std::string, std::string> env_map;

        moe::Value value;
        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        ASSERT_OK(environment.get(moe::Key("positional"), &value));
        std::string positional;
        ASSERT_OK(value.get(&positional));
        ASSERT_EQUALS(positional, "positional");
        ASSERT_OK(environment.get(moe::Key("port"), &value));
        int port;
        ASSERT_OK(value.get(&port));
        ASSERT_EQUALS(port, 5);
    }

    TEST(Parsing, PositionalMultiple) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addPositionalOption(moe::PositionalOptionDescription("positional",
                                                                    moe::StringVector, 2));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("positional1");
        argv.push_back("positional2");
        std::map<std::string, std::string> env_map;

        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        moe::Value value;
        ASSERT_OK(environment.get(moe::Key("positional"), &value));
        std::vector<std::string> positional;
        ASSERT_OK(value.get(&positional));
        std::vector<std::string>::iterator positionalit = positional.begin();
        ASSERT_EQUALS(*positionalit, "positional1");
        positionalit++;
        ASSERT_EQUALS(*positionalit, "positional2");
    }

    TEST(Parsing, PositionalMultipleExtra) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addPositionalOption(moe::PositionalOptionDescription("positional",
                                                                    moe::StringVector, 2));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("positional1");
        argv.push_back("positional2");
        argv.push_back("positional2");
        std::map<std::string, std::string> env_map;

        ASSERT_NOT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

    TEST(Parsing, PositionalMultipleUnlimited) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addPositionalOption(moe::PositionalOptionDescription("positional",
                                                                    moe::StringVector, -1));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("positional1");
        argv.push_back("positional2");
        argv.push_back("positional3");
        argv.push_back("positional4");
        argv.push_back("positional5");
        std::map<std::string, std::string> env_map;

        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        moe::Value value;
        ASSERT_OK(environment.get(moe::Key("positional"), &value));
        std::vector<std::string> positional;
        ASSERT_OK(value.get(&positional));
        std::vector<std::string>::iterator positionalit = positional.begin();
        ASSERT_EQUALS(*positionalit, "positional1");
        positionalit++;
        ASSERT_EQUALS(*positionalit, "positional2");
        positionalit++;
        ASSERT_EQUALS(*positionalit, "positional3");
        positionalit++;
        ASSERT_EQUALS(*positionalit, "positional4");
        positionalit++;
        ASSERT_EQUALS(*positionalit, "positional5");
    }

    TEST(Parsing, PositionalMultipleAndFlag) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addPositionalOption(moe::PositionalOptionDescription("positional",
                                                                    moe::StringVector, 2));
        testOpts.addOption(moe::OptionDescription("port", "port", moe::Int, "Port"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("positional1");
        argv.push_back("--port");
        argv.push_back("5");
        argv.push_back("positional2");
        std::map<std::string, std::string> env_map;

        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        moe::Value value;
        ASSERT_OK(environment.get(moe::Key("positional"), &value));
        std::vector<std::string> positional;
        ASSERT_OK(value.get(&positional));
        std::vector<std::string>::iterator positionalit = positional.begin();
        ASSERT_EQUALS(*positionalit, "positional1");
        positionalit++;
        ASSERT_EQUALS(*positionalit, "positional2");
        ASSERT_OK(environment.get(moe::Key("port"), &value));
        int port;
        ASSERT_OK(value.get(&port));
        ASSERT_EQUALS(port, 5);
    }

    TEST(Parsing, NeedArg) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("help", "help", moe::Switch, "Display help"));
        testOpts.addOption(moe::OptionDescription("port", "port", moe::Int, "Port"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--port");
        std::map<std::string, std::string> env_map;

        ASSERT_NOT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

    TEST(Parsing, BadArg) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("help", "help", moe::Switch, "Display help"));
        testOpts.addOption(moe::OptionDescription("port", "port", moe::Int, "Port"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--port");
        argv.push_back("string");
        std::map<std::string, std::string> env_map;

        ASSERT_NOT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

    TEST(Parsing, ExtraArg) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("help", "help", moe::Switch, "Display help"));
        testOpts.addOption(moe::OptionDescription("port", "port", moe::Int, "Port"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--help");
        argv.push_back("string");
        std::map<std::string, std::string> env_map;

        ASSERT_NOT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

    TEST(Style, NoSticky) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("opt", "opt,o", moe::Switch, "first opt"));
        testOpts.addOption(moe::OptionDescription("arg", "arg,a", moe::Switch, "first arg"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("-oa");
        std::map<std::string, std::string> env_map;

        ASSERT_NOT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

    TEST(Style, NoGuessing) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("help", "help", moe::Switch, "Display help"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--hel");
        std::map<std::string, std::string> env_map;

        ASSERT_NOT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

    TEST(Style, LongDisguises) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("help", "help", moe::Switch, "Display help"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("-help");
        std::map<std::string, std::string> env_map;

        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        moe::Value value;
        ASSERT_OK(environment.get(moe::Key("help"), &value));
        bool help;
        ASSERT_OK(value.get(&help));
        ASSERT_EQUALS(help, true);
    }

    TEST(Style, Verbosity) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        ASSERT_OK(testOpts.addOption(moe::OptionDescription("v", "verbose,v", moe::Switch,
                    "be more verbose (include multiple times for more verbosity e.g. -vvvvv)")));

        /* support for -vv -vvvv etc. */
        for (std::string s = "vv"; s.length() <= 12; s.append("v")) {
            ASSERT_OK(testOpts.addOption(moe::OptionDescription(s.c_str(), s.c_str(), moe::Switch,
                                                      "higher verbosity levels (hidden)", false)));
        }

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("-vvvvvv");
        std::map<std::string, std::string> env_map;

        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));

        moe::Value value;
        for (std::string s = "vv"; s.length() <= 12; s.append("v")) {
            if (s.length() == 6) {
                ASSERT_OK(environment.get(moe::Key(s), &value));
                bool verbose;
                ASSERT_OK(value.get(&verbose));
                ASSERT_EQUALS(verbose, true);
            }
            else {
                // TODO: Figure out if these are the semantics we want for switches.  Right now they
                // will always set a boolean value in the environment even if they aren't present.
                ASSERT_OK(environment.get(moe::Key(s), &value));
                bool verbose;
                ASSERT_OK(value.get(&verbose));
                ASSERT_EQUALS(verbose, false);
            }
        }
    }

    TEST(INIConfigFile, Basic) {
        OptionsParserTester parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                moe::String, "Config file to parse"));
        testOpts.addOption(moe::OptionDescription("port", "port", moe::Int, "Port"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back("default.conf");
        std::map<std::string, std::string> env_map;

        parser.setConfig("default.conf", "port=5");

        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        moe::Value value;
        ASSERT_OK(environment.get(moe::Key("port"), &value));
        int port;
        ASSERT_OK(value.get(&port));
        ASSERT_EQUALS(port, 5);
    }

    TEST(INIConfigFile, Empty) {
        OptionsParserTester parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                moe::String, "Config file to parse"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back("default.conf");
        std::map<std::string, std::string> env_map;

        parser.setConfig("default.conf", "");

        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

    TEST(INIConfigFile, Override) {
        OptionsParserTester parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                moe::String, "Config file to parse"));
        testOpts.addOption(moe::OptionDescription("port", "port", moe::Int, "Port"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back("default.conf");
        argv.push_back("--port");
        argv.push_back("6");
        std::map<std::string, std::string> env_map;

        parser.setConfig("default.conf", "port=5");

        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        moe::Value value;
        ASSERT_OK(environment.get(moe::Key("port"), &value));
        int port;
        ASSERT_OK(value.get(&port));
        ASSERT_EQUALS(port, 6);
    }

    TEST(INIConfigFile, Comments) {
        OptionsParserTester parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                moe::String, "Config file to parse"));
        testOpts.addOption(moe::OptionDescription("port", "port", moe::Int, "Port"));
        testOpts.addOption(moe::OptionDescription("str", "str", moe::String, "String"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back("default.conf");
        std::map<std::string, std::string> env_map;

        parser.setConfig("default.conf", "# port=5\nstr=NotCommented");

        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        moe::Value value;
        ASSERT_NOT_OK(environment.get(moe::Key("port"), &value));
        ASSERT_OK(environment.get(moe::Key("str"), &value));
        std::string str;
        ASSERT_OK(value.get(&str));
        ASSERT_EQUALS(str, "NotCommented");
    }

    TEST(JSONConfigFile, Basic) {
        OptionsParserTester parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                moe::String, "Config file to parse"));
        testOpts.addOption(moe::OptionDescription("port", "port", moe::Int, "Port"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back("config.json");
        std::map<std::string, std::string> env_map;

        parser.setConfig("config.json", "{ port : 5 }");

        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        moe::Value value;
        ASSERT_OK(environment.get(moe::Key("port"), &value));
        int port;
        ASSERT_OK(value.get(&port));
        ASSERT_EQUALS(port, 5);
    }

    TEST(JSONConfigFile, Empty) {
        OptionsParserTester parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                moe::String, "Config file to parse"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back("config.json");
        std::map<std::string, std::string> env_map;

        parser.setConfig("config.json", "");

        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

    TEST(JSONConfigFile, EmptyObject) {
        OptionsParserTester parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                moe::String, "Config file to parse"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back("config.json");
        std::map<std::string, std::string> env_map;

        parser.setConfig("config.json", "{}");

        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

    TEST(JSONConfigFile, Override) {
        OptionsParserTester parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                moe::String, "Config file to parse"));
        testOpts.addOption(moe::OptionDescription("port", "port", moe::Int, "Port"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back("config.json");
        argv.push_back("--port");
        argv.push_back("6");
        std::map<std::string, std::string> env_map;


        parser.setConfig("config.json", "{ port : 5 }");

        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        moe::Value value;
        ASSERT_OK(environment.get(moe::Key("port"), &value));
        int port;
        ASSERT_OK(value.get(&port));
        ASSERT_EQUALS(port, 6);
    }

    TEST(JSONConfigFile, UnregisteredOption) {
        OptionsParserTester parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                  moe::String, "Config file to parse"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back("config.json");
        std::map<std::string, std::string> env_map;

        parser.setConfig("config.json", "{ port : 5 }");

        ASSERT_NOT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

    TEST(JSONConfigFile, DuplicateOption) {
        OptionsParserTester parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                  moe::String, "Config file to parse"));
        testOpts.addOption(moe::OptionDescription("port", "port", moe::Int, "Port"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back("config.json");
        std::map<std::string, std::string> env_map;

        parser.setConfig("config.json", "{ port : 5, port : 5 }");

        ASSERT_NOT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

    TEST(JSONConfigFile, BadType) {
        OptionsParserTester parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                  moe::String, "Config file to parse"));
        testOpts.addOption(moe::OptionDescription("port", "port", moe::Int, "Port"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back("config.json");
        std::map<std::string, std::string> env_map;

        parser.setConfig("config.json", "{ port : \"string\" }");

        ASSERT_NOT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

    TEST(JSONConfigFile, Nested) {
        OptionsParserTester parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                  moe::String, "Config file to parse"));
        testOpts.addOption(moe::OptionDescription("nested.port", "port", moe::Int, "Port"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back("config.json");
        std::map<std::string, std::string> env_map;

        parser.setConfig("config.json", "{ nested : { port : 5 } }");

        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        moe::Value value;
        ASSERT_OK(environment.get(moe::Key("nested.port"), &value));
        int port;
        ASSERT_OK(value.get(&port));
        ASSERT_EQUALS(port, 5);
    }

    TEST(JSONConfigFile, StringVector) {
        OptionsParserTester parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                  moe::String, "Config file to parse"));
        testOpts.addOption(moe::OptionDescription("multival", "multival", moe::StringVector,
                                                  "Multiple Values"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back("config.json");
        std::map<std::string, std::string> env_map;

        parser.setConfig("config.json", "{ multival : [ \"val1\", \"val2\" ] }");

        moe::Value value;
        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        ASSERT_OK(environment.get(moe::Key("multival"), &value));
        std::vector<std::string> multival;
        std::vector<std::string>::iterator multivalit;
        ASSERT_OK(value.get(&multival));
        multivalit = multival.begin();
        ASSERT_EQUALS(*multivalit, "val1");
        multivalit++;
        ASSERT_EQUALS(*multivalit, "val2");
    }

    TEST(JSONConfigFile, StringVectorNonString) {
        OptionsParserTester parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                  moe::String, "Config file to parse"));
        testOpts.addOption(moe::OptionDescription("multival", "multival", moe::StringVector,
                                                  "Multiple Values"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back("config.json");
        std::map<std::string, std::string> env_map;

        parser.setConfig("config.json", "{ multival : [ 1 ] }");

        ASSERT_NOT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

    TEST(JSONConfigFile, Over16Megabytes) {
        // Test to make sure that we fail gracefully when we try to parse a JSON config file that
        // results in a BSON object larger than the current limit of 16MB
        OptionsParserTester parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                  moe::String, "Config file to parse"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back("config.json");
        std::map<std::string, std::string> env_map;

        // 1024 characters = 64 * 16
        const std::string largeString =
            "\""
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa"
            "\"";

        std::string largeConfigString;

        largeConfigString.append("{ \"largeArray\" : [ ");

        // 16mb = 16 * 1024kb = 16384
        for (int i = 0; i < 16383; i++) {
            largeConfigString.append(largeString);
            largeConfigString.append(",");
        }
        largeConfigString.append(largeString);
        largeConfigString.append(" ] }");

        parser.setConfig("config.json", largeConfigString);

        moe::Value value;
        ASSERT_NOT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

    TEST(Parsing, BadConfigFileOption) {
        OptionsParserTester parser;
        moe::Environment environment;

        moe::OptionSection testOpts;

        // TODO: Should the error be in here?
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                  moe::Int, "Config file to parse"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back("1");
        std::map<std::string, std::string> env_map;

        parser.setConfig("default.conf", "");

        ASSERT_NOT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

    TEST(ConfigFromFilesystem, JSONGood) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                  moe::String, "Config file to parse"));
        testOpts.addOption(moe::OptionDescription("port", "port", moe::Int, "Port"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back(TEST_CONFIG_PATH("good.json"));
        std::map<std::string, std::string> env_map;

        moe::Value value;
        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        ASSERT_OK(environment.get(moe::Key("port"), &value));
        int port;
        ASSERT_OK(value.get(&port));
        ASSERT_EQUALS(port, 5);
    }

    TEST(ConfigFromFilesystem, INIGood) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                  moe::String, "Config file to parse"));
        testOpts.addOption(moe::OptionDescription("port", "port", moe::Int, "Port"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back(TEST_CONFIG_PATH("good.conf"));
        std::map<std::string, std::string> env_map;

        moe::Value value;
        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
        ASSERT_OK(environment.get(moe::Key("port"), &value));
        int port;
        ASSERT_OK(value.get(&port));
        ASSERT_EQUALS(port, 5);
    }

    TEST(ConfigFromFilesystem, Empty) {
        moe::OptionsParser parser;
        moe::Environment environment;

        moe::OptionSection testOpts;
        testOpts.addOption(moe::OptionDescription("config", "config",
                                                  moe::String, "Config file to parse"));

        std::vector<std::string> argv;
        argv.push_back("binaryname");
        argv.push_back("--config");
        argv.push_back(TEST_CONFIG_PATH("empty.json"));
        std::map<std::string, std::string> env_map;

        moe::Value value;
        ASSERT_OK(parser.run(testOpts, argv, env_map, &environment));
    }

} // unnamed namespace
