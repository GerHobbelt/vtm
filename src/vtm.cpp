// Copyright (c) NetXS Group.
// Licensed under the MIT license.

#include "netxs/apps.hpp"
#include "netxs/apps/desk.hpp"
#include "vtm.hpp"
#include "netxs/apps/tile.hpp"

using namespace netxs;

enum class type { client, server, daemon, logger, branch, config };
enum class code { noaccess, noserver, nodaemon, nologger, interfer, errormsg };

int main(int argc, char* argv[])
{
    auto defaults = utf::replace_all(
        #include "vtm.xml"
        , "\n\n", "\n");
    auto whoami = type::client;
    auto params = text{};
    auto cfpath = text{};
    auto errmsg = text{};
    auto vtpipe = text{};
    auto script = text{};
    auto getopt = os::process::args{ argc, argv };
    if (getopt.starts("ssh"))//app::ssh::id))
    {
        whoami = type::branch;
        params = getopt.rest();
    }
    else while (getopt)
    {
        if (getopt.match("--svc"))
        {
            auto ok = os::process::dispatch();
            return ok ? 0 : 1;
        }
        else if (getopt.match("-b", "--", "--branch"))
        {
            whoami = type::branch;
            params = getopt.rest();
        }
        else if (getopt.match("-s", "--server"))
        {
            whoami = type::server;
        }
        else if (getopt.match("-d", "--daemon"))
        {
            whoami = type::daemon;
        }
        else if (getopt.match("-m", "--monitor"))
        {
            whoami = type::logger;
        }
        else if (getopt.match("-p", "--pipe"))
        {
            vtpipe = getopt.next();
            if (vtpipe.empty())
            {
                errmsg = "Custom pipe not specified";
                break;
            }
        }
        else if (getopt.match("-q", "--quiet"))
        {
            netxs::logger::enabled(faux);
        }
        else if (getopt.match("-l", "--listconfig"))
        {
            whoami = type::config;
        }
        else if (getopt.match("-u", "--uninstall"))
        {
            os::dtvt::initialize();
            netxs::logger::wipe();
            auto syslog = os::tty::logger();
            auto ok = os::process::uninstall();
            if (ok) log("%vtm% is uninstalled.", app::vtm::id);
            return ok ? 0 : 1;
        }
        else if (getopt.match("-i", "--install"))
        {
            os::dtvt::initialize();
            netxs::logger::wipe();
            auto syslog = os::tty::logger();
            auto ok = os::process::install();
            if (ok) log("%vtm% %ver% is installed.", app::vtm::id, app::shared::version);
            return ok ? 0 : 1;
        }
        else if (getopt.match("-c", "--config"))
        {
            cfpath = getopt.next();
            if (cfpath.empty())
            {
                errmsg = "Config file path not specified";
                break;
            }
        }
        else if (getopt.match("-?", "-h", "--help"))
        {
            os::dtvt::initialize();
            netxs::logger::wipe();
            auto syslog = os::tty::logger();
            auto vtm = os::process::binary<true>();
            auto pad = text(os::process::binary<true>().size(), ' ');
            log("\nText-based desktop environment " + text{ app::shared::version } +
                "\n"
                "\n  Syntax:"
                "\n"
                "\n    " + vtm + " [ -i | -u ] | [ -v ] | [ -? ]  |  [ -c <file>][ -l ]"
                "\n"
                "\n    " + vtm + " [ --script <commands>][ -p <name>][ -c <file>][ -q ]"
                "\n    " + pad + " [ -m | -d | -s | [ -b [ <root>]][ <arguments ...>] ]"
                "\n"
                "\n    <run commands via piped redirection> | " + vtm + " [options ...]"
                "\n"
                "\n  Options:"
                "\n"
                "\n    By default, run Detached Visual Branch with Desktop Client console as a root"
                "\n    and autorun Desktop Server daemon if it is not running."
                "\n"
                "\n    Detached Visual Branch can be seamlesly attached to the desktop via dtvt-gate."
                "\n"
                "\n    -h, -?, --help       Print command-line options."
                "\n    -v, --version        Print version."
                "\n    -l, --listconfig     Print configuration."
                "\n    -i, --install        Perform system-wide installation."
                "\n    -u, --uninstall      Perform system-wide deinstallation."
                "\n    -c, --config <file>  Specifies the settings file to load."
                "\n    -p, --pipe <name>    Specifies the desktop session connection point."
                "\n    -m, --monitor        Run desktop session monitor."
                "\n    -d, --daemon         Run desktop server as daemon."
                "\n    -s, --server         Run desktop server."
                "\n    -b, --, --branch     Run detached visual branch."
                "\n    -q, --quiet          Disable logging."
                "\n    --script <commands>  Specifies script commands to be run by the desktop when ready."
                "\n    <root>               Detached visual branch root."
                "\n    <arguments ...>      Detached visual branch root arguments."
                "\n"
                "\n  Settings loading order:"
                "\n"
                "\n    - Initialize hardcoded settings."
                "\n    - Merge with explicitly specified settings from '--config <file>'."
                "\n    - If the '--config' option is not used or <file> cannot be loaded:"
                "\n        - Merge with system-wide settings from " + os::path::expand(app::shared::sys_config).second + "."
                "\n        - Merge with user-wise settings from "   + os::path::expand(app::shared::usr_config).second + "."
                "\n        - Merge with DirectVT packet received from the parent process (dtvt-mode)."
                "\n"
                "\n  Available detached visual branch roots:"
                "\n"
                "\n    vtty   Teletype Console.           'vtm -r vtty [cui_app ...]'"
                "\n    term   Desktop Terminal.           'vtm -r term [cui_app ...]'"
                "\n    dtvt   DirectVT Console.           'vtm -r dtvt [dtvt_app ...]'"
                "\n    xlvt   DirectVT Console with TTY.  'vtm -r xlvt ssh <user@host dtvt_app ...>'"
                "\n"
                "\n    The <root> value defaults to 'vtty' if <arguments ...> is specified without <root>."
                "\n"
                "\n  The following commands have a short form:"
                "\n"
                "\n    'vtm -r xlvt ssh <user@host dtvt_app ...>' can be shortened to 'vtm ssh <user@host dtvt_app ...>'."
                "\n    'vtm -r vtty [cui_app ...]' can be shortened to 'vtm [cui_app ...]'."
                "\n"
                "\n  Scripting"
                "\n"
                "\n    Syntax: \"command1([args...])[; command2([args...]); ... commandN([args...])]\""
                "\n"
                "\n    The following characters in the script body will be de-escaped: \\e \\t \\r \\n \\a \\\" \\' \\\\"
                "\n"
                "\n    Commands:"
                "\n"
                "\n      vtm.run([<attrs>...])"
                "\n        Create and run a menu item constructed using a space-separated"
                "\n        list of attribute=<value>."
                "\n        Create and run temporary menu item constructed using default"
                "\n        attributes if no arguments specified."
                "\n"
                "\n      vtm.set(id=<id> [<attrs>...])"
                "\n        Create or override a menu item using a space-separated"
                "\n        list of attribute=<value>."
                "\n"
                "\n      vtm.del([<id>])"
                "\n        Delete the taskbar menu item by <id>."
                "\n        Delete all menu items if no <id> specified."
                "\n"
                "\n      vtm.dtvt(<dtvt_app...>)"
                "\n        Create a temporary menu item and run the specified dtvt-app."
                "\n"
                "\n      vtm.selected(<id>)"
                "\n        Set selected menu item using specified <id>."
                "\n"
                "\n      vtm.shutdown()"
                "\n        Terminate the running desktop session."
                "\n"
                "\n  Usage Examples"
                "\n"
                "\n    Run vtm desktop inside the current console:"
                "\n        vtm"
                "\n"
                "\n    Run remote vtm desktop over SSH inside the current console:"
                "\n        vtm ssh <user@server> vtm"
                "\n"
                "\n    Run Desktop Terminal inside the current console:"
                "\n        vtm -r term"
                "\n"
                "\n    Run a CUI application inside the Desktop Terminal:"
                "\n        vtm -r term </path/to/console/app>"
                "\n"
                "\n    Run a CUI application remotely over SSH:"
                "\n        vtm ssh <user@server> vtm </path/to/console/app>"
                "\n"
                "\n    Run vtm desktop and reconfigure the taskbar menu:"
                "\n        vtm --script \"vtm.del(); vtm.set(splitter id=Apps); vtm.set(id=Term)\""
                "\n"
                "\n    Reconfigure the taskbar menu of the running desktop:"
                "\n        echo \"vtm.del(); vtm.set(splitter id=Apps); vtm.set(id=Term)\" | vtm"
                "\n        echo \"vtm.set(id=user@server type=xlvt cmd='ssh <user@server> vtm')\" | vtm"
                "\n"
                "\n    Run Desktop Terminal window on the running desktop:"
                "\n        echo \"vtm.run(id=Term)\" | vtm"
                "\n        echo \"vtm.dtvt(vtm -r term)\" | vtm"
                "\n"
                "\n    Run an application window on the running desktop:"
                "\n        echo \"vtm.run(title='Console \\nApplication' cmd=</path/to/app>)\" | vtm"
                "\n"
                "\n    Run Tiling Window Manager with three terminals attached:"
                "\n        echo \"vtm.run(type=group title=Terminals cmd='v(h(Term,Term),Term)')\" | vtm"
                "\n"
                "\n    Terminate the running desktop session:"
                "\n        echo \"vtm.shutdown()\" | vtm"
                "\n"
                );
            return 0;
        }
        else if (getopt.match("-v", "--version"))
        {
            os::dtvt::initialize();
            netxs::logger::wipe();
            auto syslog = os::tty::logger();
            log(app::shared::version);
            return 0;
        }
        else if (getopt.match("--script"))
        {
            script = xml::unescape(getopt.next());
        }
        else
        {
            params = getopt.rest(); // params can't be empty at this point (see utf::quote()).
            if (params.front() == '-') errmsg = utf::concat("Unknown option '", params, "'");
            else                       whoami = type::branch;
        }
    }

    os::dtvt::initialize();
    os::dtvt::checkpoint();

    if (os::dtvt::vtmode & ui::console::redirio
     && (whoami == type::branch || whoami == type::client))
    {
        whoami = type::logger;
    }
    auto denied = faux;
    auto syslog = os::tty::logger();
    auto userid = os::env::user();
    auto prefix = vtpipe.length() ? vtpipe : utf::concat(os::path::ipc_prefix, os::process::elevated ? "!-" : "-", userid.second);;
    auto prefix_log = prefix + os::path::log_suffix;
    auto failed = [&](auto cause)
    {
        os::fail(cause == code::noaccess ? "Access denied"
               : cause == code::interfer ? "Server already running"
               : cause == code::noserver ? "Failed to start server"
               : cause == code::nologger ? "Failed to start logger"
               : cause == code::nodaemon ? "Failed to daemonize"
               : cause == code::errormsg ? errmsg.c_str()
                                         : "");
        return 1;
    };

    log(prompt::vtm, app::shared::version);
    log(getopt.show());
    if (errmsg.size())
    {
        return failed(code::errormsg);
    }
    else if (whoami == type::config)
    {
        log(prompt::resultant_settings, "\n", app::shared::load::settings<true>(defaults, cfpath, os::dtvt::config));
    }
    else if (whoami == type::logger)
    {
        log("%%Waiting for server...", prompt::main);
        auto result = std::atomic<int>{};
        auto events = os::tty::binary::logger{ [&](auto&, auto& reply)
        {
            if (reply.size() && os::dtvt::vtmode & ui::console::redirio)
            {
                os::io::send(reply);
            }
            --result;
        }};
        auto online = flag{ true };
        auto active = flag{ faux };
        auto locker = std::mutex{};
        auto syncio = std::unique_lock{ locker };
        auto buffer = std::list{ script };
        auto stream = sptr<os::ipc::socket>{};
        auto readln = os::tty::readline([&](auto line)
        {
            auto sync = std::lock_guard{ locker };
            if (active)
            {
                ++result;
                events.command.send(stream, line);
            }
            else
            {
                log("%%No server connected: %cmd%", prompt::main, utf::debase<faux, faux>(line));
                buffer.push_back(line);
            }
        }, [&]
        {
            auto sync = std::lock_guard{ locker };
            online.exchange(faux);
            if (active) while (result && active) std::this_thread::yield();
            if (active && stream) stream->shut();
        });
        while (online)
        {
            auto iolink = os::ipc::socket::open<os::role::client, faux>(prefix_log, denied);
            if (denied)
            {
                syncio.unlock();
                return failed(code::noaccess);
            }
            if (iolink)
            {
                std::swap(stream, iolink);
                result += 3;
                events.command.send(stream, utf::concat(os::process::id.first)); // First command is the monitor id.
                events.command.send(stream, os::env::add());
                events.command.send(stream, os::env::cwd());
                for (auto& line : buffer)
                {
                    ++result;
                    events.command.send(stream, line);
                }
                buffer.clear();
                active.exchange(true);
                syncio.unlock();
                directvt::binary::stream::reading_loop(stream, [&](view data){ events.s11n::sync(data); });
                syncio.lock();
                active.exchange(faux);
                break;
            }
            else
            {
                syncio.unlock();
                os::sleep(500ms);
                syncio.lock();
            }
        }
        syncio.unlock();
    }
    else if (whoami == type::branch)
    {
        auto config = app::shared::load::settings(defaults, cfpath, os::dtvt::config);
        auto shadow = params;
        auto apname = view{};
        auto aptype = text{};
        utf::to_low(shadow);
             if (shadow.starts_with(app::vtty::id))      { aptype = app::teletype::id;  apname = app::teletype::name;      }
        else if (shadow.starts_with(app::term::id))      { aptype = app::terminal::id;  apname = app::terminal::name;      }
        else if (shadow.starts_with(app::dtvt::id))      { aptype = app::dtvt::id;      apname = app::dtvt::name;      }
        else if (shadow.starts_with(app::xlvt::id))      { aptype = app::xlvt::id;      apname = app::xlvt::name;      }
        #if defined(DEBUG)
        else if (shadow.starts_with(app::calc::id))      { aptype = app::calc::id;      apname = app::calc::name;      }
        else if (shadow.starts_with(app::shop::id))      { aptype = app::shop::id;      apname = app::shop::name;      }
        else if (shadow.starts_with(app::test::id))      { aptype = app::test::id;      apname = app::test::name;      }
        else if (shadow.starts_with(app::textancy::id))  { aptype = app::textancy::id;  apname = app::textancy::name;  }
        else if (shadow.starts_with(app::settings::id))  { aptype = app::settings::id;  apname = app::settings::name;  }
        else if (shadow.starts_with(app::truecolor::id)) { aptype = app::truecolor::id; apname = app::truecolor::name; }
        #endif
        else if (shadow.starts_with("ssh"))//app::ssh::id))
        {
            params = " "s + params;
            aptype = app::xlvt::id;
            apname = app::xlvt::name;
        }
        else
        {
            params = " "s + params;
            aptype = app::vtty::id;
            apname = app::vtty::name;
        }
        log("%appname% %version%", apname, app::shared::version);
        params = utf::remain(params, ' ');
        app::shared::start(params, aptype, os::dtvt::vtmode, os::dtvt::win_sz, config);
    }
    else
    {
        auto config = app::shared::load::settings(defaults, cfpath, os::dtvt::config);
        auto client = os::ipc::socket::open<os::role::client, faux>(prefix, denied);
        auto signal = ptr::shared<os::fire>(os::process::started(prefix)); // Signaling that the server is ready for incoming connections.

             if (denied)                           return failed(code::noaccess);
        else if (whoami != type::client && client) return failed(code::interfer);
        else if (whoami == type::client && !client)
        {
            log("%%New vtm session for [%userid%]", prompt::main, userid.first);
            auto [success, successor] = os::process::fork(prefix, config.utf8());
            if (successor)
            {
                whoami = type::server;
                script = {};
            }
            else
            {
                if (success) signal->wait(10s); // Waiting for confirmation of receiving the configuration.
                else         return failed(code::noserver);
            }
        }

        if (whoami == type::client)
        {
            signal.reset();
            if (client || (client = os::ipc::socket::open<os::role::client>(prefix, denied)))
            {
                auto userinit = directvt::binary::init{};
                auto env = os::env::add();
                auto cwd = os::env::cwd();
                auto cmd = script;
                auto cfg = config.utf8();
                auto win = os::dtvt::win_sz;
                userinit.send(client, userid.first, os::dtvt::vtmode, env, cwd, cmd, cfg, win);
                os::tty::splice(client);
                return 0;
            }
            else return failed(denied ? code::noaccess : code::noserver);
        }

        if (whoami == type::daemon)
        {
            auto [success, successor] = os::process::fork(prefix, config.utf8(), script);
            if (successor)
            {
                whoami = type::server;
            }
            else 
            {
                if (success)
                {
                    signal->wait(10s); // Waiting for confirmation of receiving the configuration.
                    return 0;
                }
                else return failed(code::nodaemon);
            }
        }
        
        os::ipc::prefix = prefix;
        auto server = os::ipc::socket::open<os::role::server>(prefix, denied);
        if (!server)
        {
            if (denied) failed(code::noaccess);
            return      failed(code::noserver);
        }
        auto logger = os::ipc::socket::open<os::role::server>(prefix_log, denied);
        if (!logger)
        {
            if (denied) failed(code::noaccess);
            return      failed(code::nologger);
        }

        signal->bell(); // Signal we are started and ready for connections.
        signal.reset();

        using e2 = netxs::ui::e2;
        config.cd("/config/appearance/defaults/");
        auto domain = ui::base::create<app::vtm::hall>(server, config);
        domain->plugin<scripting::host>();
        domain->autorun();

        log("%%Session started"
          "\n      user: %userid%"
          "\n      pipe: %prefix%", prompt::main, userid.first, prefix);

        auto stdlog = std::thread{ [&]
        {
            while (auto monitor = logger->meet())
            {
                domain->run([&, monitor](auto /*task_id*/)
                {
                    auto id = text{};
                    auto active = faux;
                    auto tokens = subs{};
                    auto onecmd = eccc{};
                    auto events = os::tty::binary::logger{ [&, init = 0](auto& events, auto& cmd) mutable
                    {
                        if (active)
                        {
                            onecmd.cmd = cmd;
                            domain->SIGNAL(tier::release, scripting::events::invoke, onecmd);
                        }
                        else
                        {
                                 if (init == 0) id = cmd;
                            else if (init == 1) onecmd.env = cmd;
                            else if (init == 2)
                            {
                                active = true;
                                onecmd.cwd = cmd;
                                log("%%Monitor [%id%] connected", prompt::logs, id);
                            }
                            init++;
                        }
                        events.command.send(monitor, onecmd.cmd);
                    }};
                    auto writer = netxs::logger::attach([&](auto utf8)
                    {
                        events.logs.send(monitor, ui32{}, datetime::now(), text{ utf8 });
                    });
                    domain->LISTEN(tier::general, e2::conio::quit, deal, tokens) { monitor->shut(); };
                    os::ipc::monitors++;
                    directvt::binary::stream::reading_loop(monitor, [&](view data){ events.s11n::sync(data); });
                    os::ipc::monitors--;
                    if (id.size()) log("%%Monitor [%id%] disconnected", prompt::logs, id);
                });
            }
        }};

        auto settings = config.utf8();
        auto execline = [&](qiew line){ domain->SIGNAL(tier::release, scripting::events::invoke, onecmd, ({ .cmd = line })); };
        auto shutdown = [&]{ domain->SIGNAL(tier::general, e2::shutdown, msg, (utf::concat(prompt::main, "Shutdown on signal"))); };
        execline(script);
        auto readline = os::tty::readline(execline, shutdown);
        while (auto user = server->meet())
        {
            if (user->auth(userid.second))
            {
                domain->run([&, user, settings](auto session_id)
                {
                    auto userinit = directvt::binary::init{};
                    if (auto packet = userinit.recv(user))
                    {
                        auto id = utf::concat(*user);
                        if constexpr (debugmode) log("%%Client connected %id%", prompt::user, id);
                        auto usrcfg = eccc{ .env = packet.env, .cwd = packet.cwd, .cmd = packet.cmd, .win = packet.win };
                        auto config = xmls{ settings };
                        config.fuse(packet.cfg);
                        domain->invite(user, packet.user, packet.mode, usrcfg, config, session_id);
                        if constexpr (debugmode) log("%%Client disconnected %id%", prompt::user, id);
                    }
                });
            }
        }
        readline.stop();
        logger->stop(); // Monitor listener endpoint must be closed first to prevent reconnections.
        stdlog.join();
        domain->stop();
    }

    os::release();
}