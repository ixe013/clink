// Copyright (c) 2015 Martin Ridgers
// License: http://opensource.org/licenses/MIT

#include "pch.h"
#include "fs_fixture.h"
#include "env_fixture.h"
#include "line_editor_tester.h"

#include <core/path.h>
#include <core/settings.h>
#include <core/str_compare.h>
#include <lib/line_editor.h>
#include <lua/lua_match_generator.h>
#include <lua/lua_script_loader.h>
#include <lua/lua_state.h>

//------------------------------------------------------------------------------
TEST_CASE("Executable match generation.") {
    static const char* path_fs_desc[] = {
        "_path/spa ce.exe",
        "_path/one_path.exe",
        "_path/one_two.py",
        "_path/one_three.txt"
    };

    static const char* exec_fs_desc[] = {
        "one_dir/spa ce.exe",
        "one_dir/two_dir_local.exe",
        "one_dir/two_dir_local.txt",
        "foodir/two_dir_local.exe",
        "jumble/three.exe",
        "jumble/three-local.py",
        "one_local.exe",
        "two_local.exe",
        "one_local.txt",
        nullptr,
    };

	fs_fixture path_fs(path_fs_desc);
	fs_fixture exec_fs(exec_fs_desc);

    str<260> path_env_var(path_fs.get_root());
    path::append(path_env_var, "_path");
    const char* exec_env[] = {
        "path",     path_env_var.c_str(),
        "pathext",  ".exe;.py",
        nullptr,
    };
    env_fixture env(exec_env);

    lua_state lua;
    lua_match_generator lua_generator(lua);
    lua_load_script(lua, app, exec);

    line_editor_tester tester;
    tester.get_editor()->add_generator(lua_generator);

    settings::find("exec.cwd")->set("0");
    settings::find("exec.dirs")->set("0");

    SECTION("Nothing") {
        tester.set_input("abc123");
        tester.set_expected_matches();
        tester.run();
    }

    SECTION("PATH single") {
        tester.set_input("one_p");
        tester.set_expected_matches("one_path.exe");
        tester.run();
    }

    SECTION("PATH case mapped") {
        str_compare_scope _(str_compare_scope::relaxed);

        tester.set_input("one-p");
        tester.set_expected_matches("one_path.exe");
        tester.run();
    }

    SECTION("PATH matches") {
        tester.set_input("one_");
        tester.set_expected_matches("one_path.exe", "one_two.py");
        tester.run();
    }

#if MODE4 // tester's shell != cmd.exe
    SECTION("cmd.exe commands") {
        tester.set_input("p");
        tester.set_expected_matches("path", "pause", "popd", "prompt", "pushd");
        tester.run();
    }
#endif

    SECTION("Relative path") {
        tester.set_input(".\\");
        tester.set_expected_matches(
			"one_local.exe", "two_local.exe",
            "one_dir\\", "foodir\\", "jumble\\"
		);
        tester.run();
    }

    SECTION("Relative path dir") {
        tester.set_input(".\\foodir\\");
        tester.set_expected_matches("two_dir_local.exe");
        tester.run();
    }

    SECTION("Relative path dir (with '_')") {
        tester.set_input(".\\one_dir\\t");
        tester.set_expected_matches("two_dir_local.exe");
        tester.run();
    }

    SECTION("Spaces (path)") {
        tester.set_input("spa");
        tester.set_expected_matches("spa ce.exe");
        tester.run();
    }

    SECTION("Spaces (relative)") {
        tester.set_input(".\\one_dir\\spa");
        tester.set_expected_matches("spa ce.exe");
        tester.run();
    }

    SECTION("Last char . 1") {
        tester.set_input("one_path.");
        tester.set_expected_matches("one_path.exe");
        tester.run();
    }

    SECTION("Last char . 2") {
        tester.set_input("jumble\\three.");
        tester.set_expected_matches("three.exe");
        tester.run();
    }

    SECTION("Last char -") {
        tester.set_input("one_local-");
        tester.set_expected_matches();
        tester.run();
    }

    SECTION("Style") {
        settings::find("exec.cwd")->set("1");

        SECTION("Cwd (no dirs)") {
            settings::find("exec.dirs")->set("0");

            tester.set_input("one_");
            tester.set_expected_matches("one_local.exe", "one_path.exe", "one_two.py");
            tester.run();
        }

        SECTION("Style - cwd (all)") {
            str_compare_scope _(str_compare_scope::relaxed);

            settings::find("exec.dirs")->set("1");

            tester.set_input("one-\t");
            tester.set_expected_matches("one_local.exe", "one_path.exe",
                "one_two.py", "one_dir\\");
            tester.run();
        }
    }

    SECTION("Space prefix") {
        tester.get_editor()->add_generator(file_match_generator());

        settings::find("exec.space_prefix")->set("1");

        SECTION("None") {
            tester.set_input("one_");
            tester.set_expected_matches("one_path.exe", "one_two.py");
            tester.run();
        }

        SECTION("Space") {
            tester.set_input(" one_");
            tester.set_expected_matches("one_dir\\", "one_local.txt", "one_local.exe");
            tester.run();
        }

        SECTION("Spaces") {
            tester.set_input("   one_");
            tester.set_expected_matches("one_dir\\", "one_local.txt", "one_local.exe");
            tester.run();
        }
    }

#if MODE4 // separators not yet implemented.
    clink.test.test_output(
        "separator | 1",
        "nullcmd | one_p",
        "nullcmd | one_path.exe "
    )

    clink.test.test_output(
        "separator | 2",
        "nullcmd |one_p",
        "nullcmd |one_path.exe "
    )

    clink.test.test_matches(
        "separator & 1",
        "nullcmd & one_",
        { "one_path.exe", "one_two.py" }
    )

    clink.test.test_matches(
        "separator & 2",
        "nullcmd &one_",
        { "one_path.exe", "one_two.py" }
    )

    clink.test.test_output(
        "separator && 1",
        "nullcmd && one_p",
        "nullcmd && one_path.exe "
    )

    clink.test.test_output(
        "separator && 2",
        "nullcmd &&one_p",
        "nullcmd &&one_path.exe "
    )

    clink.test.test_matches(
        "separator false positive",
        "nullcmd \"&&\" o\t",
        { "one_local.exe", "one_local.txt", "one_dir\\" }
    )
#endif // MODE4
}