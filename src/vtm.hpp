// Copyright (c) NetXS Group.
// Licensed under the MIT license.

#pragma once

#include "netxs/desktopio/application.hpp"
#include "netxs/apps/desk.hpp"

namespace netxs::app::vtm
{
    static constexpr auto id = "vtm";
    static constexpr auto desc = " vtm:";

    struct link
    {
        using sptr = netxs::sptr<base>;
        text menuid{};
        text header{};
        text footer{};
        rect square{};
        bool forced{};
        sptr applet{};
    };

    static constexpr auto attr_id       = "id";
    static constexpr auto attr_alias    = "alias";
    static constexpr auto attr_hidden   = "hidden";
    static constexpr auto attr_label    = "label";
    static constexpr auto attr_notes    = "notes";
    static constexpr auto attr_title    = "title";
    static constexpr auto attr_footer   = "footer";
    static constexpr auto attr_bgc      = "bgc";
    static constexpr auto attr_fgc      = "fgc";
    static constexpr auto attr_winsize  = "winsize";
    static constexpr auto attr_wincoor  = "wincoor";
    static constexpr auto attr_focused  = "focused";
    static constexpr auto attr_slimmenu = "slimmenu";
    static constexpr auto attr_hotkey   = "hotkey";
    static constexpr auto attr_type     = "type";
    static constexpr auto attr_cwd      = "cwd";
    static constexpr auto attr_param    = "param";
    static constexpr auto attr_splitter = "splitter";
    static constexpr auto attr_config   = "config";

    static constexpr auto path_item     = "/config/menu/item";
    static constexpr auto path_autorun  = "/config/menu/autorun/item";
    static constexpr auto path_viewport = "/config/menu/viewport/coor";

    struct events
    {
        EVENTPACK( events, ui::e2::extra::slot1 )
        {
            EVENT_XS( newapp  , link       ), // request: create new object using specified meniid
            EVENT_XS( handoff , link       ), // general: attach spcified intance and return sptr<base>.
            EVENT_XS( attached, sptr<base> ), // anycast: inform that the object tree is attached to the world
            GROUP_XS( d_n_d   , sptr<base> ), // drag&drop functionality. See tiling manager empty slot and pro::d_n_d.

            SUBSET_XS(d_n_d)
            {
                EVENT_XS( ask  , sptr<base> ),
                EVENT_XS( abort, sptr<base> ),
                EVENT_XS( drop , link       ),
            };
        };
    };

    namespace pro
    {
        using namespace netxs::ui::pro;

        // pro: Provides functionality for manipulating objects with a frame structure.
        class frame
            : public skill
        {
            using skill::boss,
                  skill::memo;

            subs  link;
            robot robo;
            zpos  seat;

        public:
            frame(base&&) = delete;
            frame(base& boss, zpos z_order = zpos::plain) : skill{ boss },
                robo{ boss    },
                seat{ z_order }
            {
                boss.LISTEN(tier::release, e2::form::upon::vtree::attached, parent, memo)
                {
                    parent->LISTEN(tier::preview, e2::form::global::lucidity, alpha, link)
                    {
                        boss.SIGNAL(tier::preview, e2::form::global::lucidity, alpha);
                    };
                    parent->LISTEN(tier::preview, e2::form::layout::convey, convey_data, link)
                    {
                        convey(convey_data.delta, convey_data.stuff);
                    };
                    parent->LISTEN(tier::preview, e2::form::layout::shift, delta, link)
                    {
                        //boss.base::coor += delta;
                        boss.moveby(delta);
                    };
                    parent->LISTEN(tier::preview, e2::form::upon::vtree::detached, p, link)
                    {
                        frame::link.clear();
                    };
                    boss.SIGNAL(tier::release, e2::form::prop::zorder, seat);
                };
                boss.LISTEN(tier::preview, e2::form::prop::zorder, order)
                {
                    seat = order;
                    boss.SIGNAL(tier::release, e2::form::prop::zorder, seat);
                };
                boss.LISTEN(tier::preview, e2::form::layout::expose, boss, memo)
                {
                    expose();
                };
                boss.LISTEN(tier::preview, hids::events::mouse::button::click::left, gear, memo)
                {
                    expose();
                };
                boss.LISTEN(tier::preview, hids::events::mouse::button::click::right, gear, memo)
                {
                    expose();
                };
                boss.LISTEN(tier::preview, e2::form::layout::appear, newpos, memo)
                {
                    appear(newpos);
                };
                //boss.LISTEN(tier::preview, e2::form::upon::moved, delta, memo)
                //{
                //    bubble();
                //};
                boss.LISTEN(tier::preview, e2::form::upon::changed, delta, memo)
                {
                    bubble();
                };
                boss.LISTEN(tier::preview, hids::events::mouse::button::down::any, gear, memo)
                {
                    robo.pacify();
                };
                boss.LISTEN(tier::release, e2::form::drag::pull::any, gear, memo)
                {
                    if (gear)
                    {
                        auto deed = boss.bell::template protos<tier::release>();
                        switch (deed)
                        {
                            case e2::form::drag::pull::left.id:
                            case e2::form::drag::pull::leftright.id:
                            {
                                auto delta = gear.delta.get();
                                boss.base::anchor = gear.coord; // See pro::align unbind.
                                boss.base::moveby(delta);
                                boss.SIGNAL(tier::preview, e2::form::upon::changed, delta);
                                gear.dismiss();
                                break;
                            }
                            default: break;
                        }
                    }
                };
                boss.LISTEN(tier::release, e2::form::upon::dragged, gear, memo)
                {
                    if (gear.meta(hids::anyCtrl))
                    {
                        robo.actify(gear.fader<quadratic<twod>>(2s), [&](auto x)
                        {
                            boss.base::moveby(x);
                            boss.strike();
                        });
                    }
                    else
                    {
                        auto boundary = gear.area();
                        robo.actify(gear.fader<quadratic<twod>>(2s), [&, boundary](auto x)
                        {
                            convey(x, boundary);
                            boss.strike();
                        });
                    }
                };
                boss.LISTEN(tier::release, hids::events::mouse::button::click::right, gear, memo)
                {
                    auto& area = boss.base::area();
                    auto coord = gear.coord + area.coor;
                    if (!area.hittest(coord))
                    {
                        pro::focus::set(boss.This(), gear.id, pro::focus::solo::on, pro::focus::flip::off);
                        appear(coord);
                    }
                    gear.dismiss();
                };
            };

            // pro::frame: Fly to the specified position.
            void appear(twod const& target)
            {
                auto& screen = boss.base::area();
                auto  oldpos = screen.coor;
                auto  newpos = target - screen.size / 2;;

                auto path = newpos - oldpos;
                auto time = skin::globals().switching;
                auto init = 0;
                auto func = constlinearAtoB<twod>(path, time, init);

                robo.pacify();
                robo.actify(func, [&](twod& x) { boss.base::moveby(x); boss.strike(); });
            }
            /*
            // pro::frame: Search for a non-overlapping form position in
            //             the visual tree along a specified direction.
            rect bounce(rect const& block, twod const& dir)
            {
                auto result = block.rotate(dir);
                auto parity = std::abs(dir.x) > std::abs(dir.y);

                for (auto xy : { parity, !parity })
                {
                    auto ray = result;
                    ray.coor[xy] += ray.size[xy];
                    ray.size[xy] = dir[xy] > 0 ? std::numeric_limits<int>::max()
                                               : std::numeric_limits<int>::min();

                    if (auto shadow = ray.trunc(boss.base::size))
                    {
                        auto direct = shadow.rotate(dir);
                        auto nearby = direct.coor[xy] + direct.size[xy];

                        foreach(boss.branch, boss.status.is.visible, [&](auto item)
                                {
                                    if (auto s = shadow.clip(item->square()))
                                    {
                                        auto next = dir[xy] > 0 ? s.coor[xy] : -(s.coor[xy] + s.size[xy]);
                                        if (next < nearby) nearby = next;
                                    }
                                });

                        result.size[xy] = (dir[xy] > 0 ? nearby : -nearby) - result.coor[xy];
                    }
                }

                return result;
            }
            */
            // pro::frame: Move the form no further than the parent canvas.
            void convey(twod const& delta, rect const& boundary)//, bool notify = true)
            {
                auto& r0 = boss.base::area();
                if (delta && r0.clip(boundary))
                {
                    auto r1 = r0;
                    auto r2 = boundary;
                    r1.coor -= r2.coor;

                    auto c = r1.rotate(-delta);
                    auto s = r2.size;
                    auto o = delta.less(dot_00, dot_00, dot_11);
                    if ((s + o).twod::inside(c.coor))
                    {
                        c.coor = std::clamp(c.coor + delta, dot_00, s);
                        auto newcoor = c.normalize().coor + r2.coor;
                        boss.moveto(newcoor);
                    }
                    else if (!r2.clip(r0))
                    {
                        boss.moveby(delta);
                    }
                }
            }
            // pro::frame: Check if it is under the rest, and moves it to the top of the visual tree.
            //             Return "true" if it is NOT under the rest.
            void expose(bool subsequent = faux)
            {
                if (auto parent_ptr = boss.parent())
                {
                    parent_ptr->SIGNAL(tier::release, e2::form::layout::expose, boss);
                }
                //return boss.status.exposed;
            }
            // pro::frame: Place the form in front of the visual tree among neighbors.
            void bubble()
            {
                if (auto parent_ptr = boss.parent())
                {
                    parent_ptr->SIGNAL(tier::release, e2::form::layout::bubble, boss);
                }
            }
        };

        // pro: Form generator.
        class maker
            : public skill
        {
            using skill::boss,
                  skill::memo;

            cell mark;

            struct slot_t
            {
                rect slot{};
                twod step{};
                twod init{};
                bool ctrl{};
            };
            std::unordered_map<id_t, slot_t> slots;
            ansi::esc coder;

            void check_modifiers(hids& gear)
            {
                auto& data = slots[gear.id];
                auto state = !!gear.meta(hids::anyCtrl);
                if (data.ctrl != state)
                {
                    data.ctrl = state;
                    boss.deface(data.slot);
                }
            }
            void handle_init(hids& gear)
            {
                if (gear.capture(boss.bell::id))
                {
                    auto& data = slots[gear.id];
                    auto& slot = data.slot;
                    auto& init = data.init;
                    auto& step = data.step;

                    data.ctrl = gear.meta(hids::anyCtrl);
                    slot.coor = init = step = gear.coord;
                    slot.size = dot_00;
                    boss.deface(slot);
                    gear.dismiss();
                }
            }
            void handle_pull(hids& gear)
            {
                if (gear.captured(boss.bell::id))
                {
                    check_modifiers(gear);
                    auto& data = slots[gear.id];
                    auto& slot = data.slot;
                    auto& init = data.init;
                    auto& step = data.step;

                    step += gear.delta.get();
                    slot.coor = std::min(init, step);
                    slot.size = std::max(std::abs(step - init), dot_00);
                    boss.deface(slot);
                    gear.dismiss();
                }
            }
            void handle_drop(hids& gear)
            {
                if (gear.captured(boss.bell::id))
                {
                    slots.erase(gear.id);
                    gear.dismiss();
                    gear.setfree();
                }
            }
            void handle_stop(hids& gear)
            {
                if (gear.captured(boss.bell::id))
                {
                    check_modifiers(gear);
                    auto& data = slots[gear.id];
                    if (data.slot)
                    {
                        gear.slot = data.slot;
                        gear.slot.coor += boss.base::coor();
                        gear.slot_forced = true;
                        boss.RISEUP(tier::request, e2::form::proceed::createby, gear);
                    }
                    slots.erase(gear.id);
                    gear.dismiss();
                    gear.setfree();
                }
            }

        public:
            maker(base&&) = delete;
            maker(base& boss)
                : skill{ boss },
                   mark{ skin::color(tone::selector) }
            {
                using drag = hids::events::mouse::button::drag;

                boss.LISTEN(tier::preview, hids::events::keybd::data::post, gear, memo)
                {
                    if (gear.captured(boss.bell::id)) check_modifiers(gear);
                };

                //todo unify - args... + template?
                //middle button
                //todo revise boss.LISTEN(tier::preview, drag::start::middle, gear, memo)
                boss.LISTEN(tier::release, drag::start::middle, gear, memo)
                {
                    handle_init(gear);
                };
                boss.LISTEN(tier::release, drag::pull::middle, gear, memo)
                {
                    handle_pull(gear);
                };
                boss.LISTEN(tier::release, drag::cancel::middle, gear, memo)
                {
                    handle_drop(gear);
                };
                boss.LISTEN(tier::release, drag::stop::middle, gear, memo)
                {
                    handle_stop(gear);
                };

                //todo unify
                //right button
                boss.LISTEN(tier::release, drag::start::right, gear, memo)
                {
                    handle_init(gear);
                };
                boss.LISTEN(tier::release, drag::pull::right, gear, memo)
                {
                    handle_pull(gear);
                };
                boss.LISTEN(tier::release, drag::cancel::right, gear, memo)
                {
                    handle_drop(gear);
                };
                boss.LISTEN(tier::release, drag::stop::right, gear, memo)
                {
                    handle_stop(gear);
                };

                boss.LISTEN(tier::general, hids::events::halt, gear, memo)
                {
                    handle_drop(gear);
                };

                boss.LISTEN(tier::release, e2::postrender, canvas, memo)
                {
                    //todo Highlighted area drawn twice
                    auto offset = boss.coor() - canvas.coor();
                    for (auto const& [key, data] : slots)
                    {
                        auto slot = data.slot;
                        slot.coor += offset;
                        if (auto area = canvas.area().clip<true>(slot))
                        {
                            if (data.ctrl)
                            {
                                area.coor -= dot_11;
                                area.size += dot_22;
                                auto mark = skin::color(tone::kb_focus);
                                auto fill = [&](cell& c) { c.fuse(mark); };
                                canvas.cage(area, dot_11, fill);
                                coder.wrp(wrap::off).add("capture area: ", slot);
                                //todo optimize para
                                auto caption = para(coder);
                                coder.clear();
                                auto header = *caption.lyric;
                                auto coor = area.coor + canvas.coor();
                                coor.y--;
                                header.move(coor);
                                canvas.fill(header, cell::shaders::contrast);
                            }
                            else
                            {
                                auto temp = canvas.view();
                                canvas.view(area);
                                canvas.fill(area, [&](cell& c) { c.fuse(mark); c.und(faux); });
                                canvas.blur(10);
                                coder.wrp(wrap::off).add(' ').add(slot.size.x).add(" × ").add(slot.size.y).add(' ');
                                //todo optimize para
                                auto caption = para(coder);
                                coder.clear();
                                auto header = *caption.lyric;
                                auto coor = area.coor + area.size + canvas.coor();
                                coor.x -= caption.length() - 1;
                                header.move(coor);
                                canvas.fill(header, cell::shaders::contrast);
                                canvas.view(temp);
                            }
                        }
                    }
                };
            }
        };

        // pro: Drag&drop functionality.
        class d_n_d
            : public skill
        {
            using wptr = netxs::wptr<base>;
            using skill::boss,
                  skill::memo;

            id_t under;
            bool drags;
            twod coord;
            wptr cover;

            void proceed(bool keep)
            {
                drags = faux;
                boss.SIGNAL(tier::anycast, e2::form::prop::lucidity, 0xFF); // Make target opaque.
                if (auto dest_ptr = cover.lock())
                {
                    auto& dest = *dest_ptr;
                    if (keep)
                    {
                        boss.SIGNAL(tier::preview, vtm::events::d_n_d::drop, what, ()); // Take core.
                        dest.SIGNAL(tier::release, vtm::events::d_n_d::drop, what); // Pass core.
                        boss.base::detach(); // The object kills itself.
                    }
                    else dest.SIGNAL(tier::release, vtm::events::d_n_d::abort, boss.This());
                }
                cover.reset();
                under = {};
            }

        public:
            d_n_d(base&&) = delete;
            d_n_d(base& boss)
                : skill{ boss },
                  drags{ faux },
                  under{      }
            {
                boss.LISTEN(tier::release, hids::events::mouse::button::drag::start::any, gear, memo)
                {
                    if (boss.size().inside(gear.coord) && !gear.kbmod())
                    if (drags || !gear.capture(boss.id)) return;
                    {
                        drags = true;
                        coord = gear.coord;
                        under = {};
                    }
                };
                boss.LISTEN(tier::release, hids::events::mouse::button::drag::pull::any, gear, memo)
                {
                    if (!drags) return;
                    if (gear.kbmod()) proceed(faux);
                    else              coord = gear.coord - gear.delta.get();
                };
                boss.LISTEN(tier::release, hids::events::mouse::button::drag::stop::any, gear, memo)
                {
                    if (!drags) return;
                    if (gear.kbmod()) proceed(faux);
                    else              proceed(true);
                    gear.setfree();
                };
                boss.LISTEN(tier::release, hids::events::mouse::button::drag::cancel::any, gear, memo)
                {
                    if (!drags) return;
                    if (gear.kbmod()) proceed(faux);
                    else              proceed(true);
                    gear.setfree();
                };
                boss.LISTEN(tier::release, e2::render::prerender, parent_canvas, memo)
                {
                    if (!drags) return;
                    auto full = parent_canvas.face::full();
                    auto size = parent_canvas.core::size();
                    auto coor = full.coor + coord;
                    if (size.inside(coor))
                    {
                        auto& c = parent_canvas[coor];
                        auto new_under = c.link();
                        if (under != new_under)
                        {
                            auto object = vtm::events::d_n_d::ask.param();
                            if (auto old_object = bell::getref<base>(under))
                            {
                                old_object->RISEUP(tier::release, vtm::events::d_n_d::abort, object);
                            }
                            if (auto new_object = bell::getref<base>(new_under))
                            {
                                new_object->RISEUP(tier::release, vtm::events::d_n_d::ask, object);
                            }
                            boss.SIGNAL(tier::anycast, e2::form::prop::lucidity, object ? 0x80
                                                                                        : 0xFF); // Make it semi-transparent on success and opaque otherwise.
                            cover = object;
                            under = new_under;
                        }
                    }
                };
            }
        };
    }

    // vtm: User gate.
    struct gate
        : public ui::gate
    {
        pro::maker maker{*this }; // gate: Form generator.

        gate(sptr<pipe> uplink, view userid, si32 vtmode, xmls& config, si32 session_id)
            : ui::gate{ uplink, vtmode, config, userid, session_id, true }
        {
            //todo local=>nexthop
            local = faux;

            LISTEN(tier::release, hids::events::keybd::data::post, gear, tokens)
            {
                if (gear)
                {
                    if (true /*...*/) //todo lookup taskbar kb shortcuts
                    {
                        //...
                        //pro::focus::set(applet, gear.id, pro::focus::solo::on, pro::focus::flip::off);
                        
                    }
                    else
                    {
                        //...
                    }
                }
            };
            LISTEN(tier::preview, hids::events::keybd::data::any, gear, tokens)
            {
                //todo deprecated
                //todo unify
                auto& keystrokes = gear.keystrokes;
                auto pgup = keystrokes == "\033[5;5~"s
                        || (keystrokes == "\033[5~"s && gear.meta(hids::anyCtrl));
                auto pgdn = keystrokes == "\033[6;5~"s
                        || (keystrokes == "\033[6~"s && gear.meta(hids::anyCtrl));
                if (pgup || pgdn)
                {
                    auto item_ptr = e2::form::layout::goprev.param();
                    if (pgdn) this->RISEUP(tier::request, e2::form::layout::goprev, item_ptr); // Take prev item
                    else      this->RISEUP(tier::request, e2::form::layout::gonext, item_ptr); // Take next item

                    if (item_ptr)
                    {
                        auto& area = item_ptr->area();
                        auto center = area.coor + (area.size / 2);
                        this->SIGNAL(tier::release, e2::form::layout::shift, center);
                        pro::focus::set(item_ptr, gear.id, pro::focus::solo::on, pro::focus::flip::off);
                    }
                    //gear.dismiss();
                    this->bell::expire<tier::preview>(); //todo temp
                    gear.set_handled(true);
                }
            };

            //todo move it to the desk (dragging)
            mouse.draggable<hids::buttons::leftright>(true);
            mouse.draggable<hids::buttons::left>(true);
            LISTEN(tier::release, e2::form::drag::start::any, gear, tokens)
            {
                robot.pacify();
            };
            LISTEN(tier::release, e2::form::drag::pull::any, gear, tokens)
            {
                base::moveby(-gear.delta.get());
                base::deface();
            };
            LISTEN(tier::release, e2::form::drag::stop::any, gear, tokens)
            {
                robot.pacify();
                robot.actify(gear.fader<quadratic<twod>>(2s), [&](auto& x)
                {
                    base::moveby(-x);
                    base::deface();
                });
            };
            LISTEN(tier::release, e2::form::layout::shift, newpos, tokens)
            {
                this->SIGNAL(tier::request, e2::form::prop::viewport, viewport, ());
                auto oldpos = viewport.coor + (viewport.size / 2);

                auto path = oldpos - newpos;
                auto time = skin::globals().switching;
                auto init = 0;
                auto func = constlinearAtoB<twod>(path, time, init);

                robot.pacify();
                robot.actify(func, [&](auto& x)
                {
                    base::moveby(-x);
                    base::strike();
                });
            };
        }

        void rebuild_scene(base& world, bool damaged) override
        {
            auto& canvas = input.xmap;
            if (damaged)
            {
                canvas.wipe(world.bell::id);
                if (props.background_image.size())
                {
                    //todo cache background
                    canvas.tile(props.background_image, cell::shaders::fuse);
                }
                world.redraw(canvas); // Put the rest of the world on my canvas.
                if (applet && !fullscreen) // Render main menu/application.
                {
                    //todo too hacky, unify
                    if (props.glow_fx) canvas.render(applet, base::coor()); // Render the main menu twice to achieve the glow effect.
                                       canvas.render(applet, base::coor());
                }
            }
            _rebuild_scene(damaged);
        }
    };

    // vtm: Desktopio Workspace.
    struct hall
        : public host
    {
    private:
        struct node // hall: Adapter for the object that going to be attached to the world.
        {
            using sptr = netxs::sptr<base>;

            bool highlighted = faux;
            si32 active = 0;
            tone color = { tone::brighter, tone::shadower };
            rect region;
            sptr object;
            id_t obj_id;
            zpos z_order = zpos::plain;
            subs tokens;

            node(sptr item)
                : object{ item }
            {
                auto& inst = *item;
                obj_id = inst.bell::id;

                inst.LISTEN(tier::release, e2::form::prop::zorder, order, tokens)
                {
                    z_order = order;
                };
                inst.LISTEN(tier::release, e2::size::any, size, tokens)
                {
                    region.size = size;
                };
                inst.LISTEN(tier::release, e2::coor::any, coor, tokens)
                {
                    region.coor = coor;
                };
                inst.LISTEN(tier::release, e2::form::state::mouse, state, tokens)
                {
                    active = state;
                };
                inst.LISTEN(tier::release, e2::form::state::highlight, state, tokens)
                {
                    highlighted = state;
                };
                inst.LISTEN(tier::release, e2::form::state::color, new_color, tokens)
                {
                    color = new_color;
                };

                inst.SIGNAL(tier::request, e2::size::set, region.size);
                inst.SIGNAL(tier::request, e2::coor::set, region.coor);
                inst.SIGNAL(tier::request, e2::form::state::mouse, active);
                inst.SIGNAL(tier::request, e2::form::state::color, color);
            }
            // hall::node: Check equality.
            bool equals(id_t id)
            {
                return obj_id == id;
            }
            // hall::node: Draw a navigation string.
            void fasten(face& canvas)
            {
                auto window = canvas.area();
                auto origin = window.size / 2;
                //auto origin = twod{ 6, window.size.y - 3 };
                auto offset = region.coor - window.coor;
                auto center = offset + (region.size / 2);
                //header.usable = window.overlap(region);
                auto is_active = active || highlighted;
                auto& grade = skin::grade(is_active ? color.active
                                                    : color.passive);
                auto pset = [&](twod const& p, uint8_t k)
                {
                    //canvas[p].fuse(grade[k], obj_id, p - offset);
                    //canvas[p].fuse(grade[k], obj_id);
                    canvas[p].link(obj_id).bgc().mix_one(grade[k].bgc());
                };
                window.coor = dot_00;
                netxs::online(window, origin, center, pset);
            }
            // hall::node: Visualize the underlying object.
            template<bool Post = true>
            void render(face& canvas)
            {
                canvas.render<Post>(*object);
            }
            void postrender(face& canvas)
            {
                object->SIGNAL(tier::release, e2::postrender, canvas);
            }
        };
        struct list // hall: List of objects that can be reordered, etc.
        {
            std::list<sptr<node>> items;

            template<class D>
            auto search(D head, D tail, id_t id)
            {
                if (items.size())
                {
                    auto test = [id](auto& a) { return a->equals(id); };
                    return std::find_if(head, tail, test);
                }
                return tail;
            }

            operator bool () { return items.size(); }
            auto size()      { return items.size(); }
            auto back()      { return items.back()->object; }
            void append(sptr<base> item)
            {
                items.push_back(ptr::shared<node>(item));
            }
            //hall::list: Draw backpane for spectators.
            void prerender(face& canvas)
            {
                if (size() > 1)
                for (auto& item : items) item->fasten(canvas); // Draw strings.
                for (auto& item : items) item->render<faux>(canvas); // Draw shadows without postrendering.
            }
            //hall::list: Draw windows.
            void render(face& canvas)
            {
                for (auto& item : items) item->fasten(canvas);
                //todo optimize
                for (auto& item : items) if (item->z_order == zpos::backmost) item->render(canvas);
                for (auto& item : items) if (item->z_order == zpos::plain   ) item->render(canvas);
                for (auto& item : items) if (item->z_order == zpos::topmost ) item->render(canvas);
            }
            //hall::list: Draw spectator's mouse pointers.
            void postrender(face& canvas)
            {
                for (auto& item : items) item->postrender(canvas);
            }
            //hall::list: Delete all items.
            void reset()
            {
                items.clear();
            }
            rect remove(id_t id)
            {
                auto area = rect{};
                auto head = items.begin();
                auto tail = items.end();
                auto item = search(head, tail, id);
                if (item != tail)
                {
                    area = (**item).region;
                    items.erase(item);
                }
                return area;
            }
            rect bubble(id_t id)
            {
                auto head = items.rbegin();
                auto tail = items.rend();
                auto item = search(head, tail, id);

                if (item != head && item != tail)
                {
                    auto& area = (**item).region;
                    if (!area.clip((**std::prev(item)).region))
                    {
                        auto shadow = *item;
                        items.erase(std::next(item).base());

                        while (--item != head
                            && !area.clip((**std::prev(item)).region))
                        { }

                        items.insert(item.base(), shadow);
                        return area;
                    }
                }

                return rect_00;
            }
            rect expose(id_t id)
            {
                auto head = items.rbegin();
                auto tail = items.rend();
                auto item = search(head, tail, id);

                if (item != head && item != tail)
                {
                    auto shadow = *item;
                    items.erase(std::next(item).base());
                    items.push_back(shadow);
                    return shadow->region;
                }

                return rect_00;
            }
            auto rotate_next()
            {
                items.push_back(items.front());
                items.pop_front();
                return items.back();
            }
            auto rotate_prev()
            {
                items.push_front(items.back());
                items.pop_back();
                return items.back();
            }
        };
        struct depo // hall: Actors registry.
        {
            sptr<desk::apps> apps_ptr = ptr::shared(desk::apps{});
            sptr<desk::usrs> usrs_ptr = ptr::shared(desk::usrs{});
            sptr<desk::menu> menu_ptr = ptr::shared(desk::menu{});
            desk::apps& apps = *apps_ptr;
            desk::usrs& usrs = *usrs_ptr;
            desk::menu& menu = *menu_ptr;

            void append(sptr<base> user)
            {
                usrs.push_back(user);
            }
            auto remove(sptr<base> item_ptr)
            {
                auto found = faux;
                for (auto& [class_id, fxd_app_list] : apps) // Remove app.
                {
                    auto& [fixed, app_list] = fxd_app_list;
                    auto head = app_list.begin();
                    auto tail = app_list.end();
                    auto iter = std::find_if(head, tail, [&](auto& c) { return c == item_ptr; });
                    if (iter != tail)
                    {
                        app_list.erase(iter);
                        if (app_list.empty() && !fixed)
                        {
                            apps.erase(class_id);
                        }
                        found = true;
                        break;
                    }
                }
                { // Remove user.
                    auto head = usrs.begin();
                    auto tail = usrs.end();
                    auto iter = std::find_if(head, tail, [&](auto& c){ return c == item_ptr; });
                    if (iter != tail)
                    {
                        usrs.erase(iter);
                        found = true;
                    }
                }
                return found;
            }
            void reset()
            {
                apps.clear();
            }
        };

        using idls = std::vector<id_t>;

        list items; // hall: Child visual tree.
        list users; // hall: Scene spectators.
        depo dbase; // hall: Actors registry.
        twod vport; // hall: Last user's viewport position.

        auto window(link& what)
        {
            return ui::cake::ctor()
                ->plugin<pro::d_n_d>()
                ->plugin<pro::title>(what.header, what.footer)
                ->plugin<pro::limit>(dot_11, twod{ 400,200 }) //todo unify, set via config
                ->plugin<pro::sizer>()
                ->plugin<pro::frame>()
                ->plugin<pro::light>()
                ->plugin<pro::align>()
                ->plugin<pro::focus>()
                ->invoke([&](auto& boss)
                {
                    boss.base::kind(base::reflow_root); //todo unify -- See base::reflow()
                    boss.LISTEN(tier::preview, vtm::events::d_n_d::drop, what, -, (menuid = what.menuid))
                    {
                        if (auto applet = boss.pop_back())
                        {
                            boss.SIGNAL(tier::request, e2::form::prop::ui::header, what.header);
                            boss.SIGNAL(tier::request, e2::form::prop::ui::footer, what.footer);
                            what.applet = applet;
                            what.menuid = menuid;
                        }
                    };
                    boss.LISTEN(tier::release, hids::events::mouse::button::dblclick::left, gear)
                    {
                        boss.RISEUP(tier::release, e2::form::maximize, gear);
                        gear.dismiss();
                    };
                    boss.LISTEN(tier::release, hids::events::mouse::button::click::left, gear)
                    {
                        auto area = boss.base::area();
                        auto home = rect{ -dot_21, area.size + dot_21 * 2}; // Including resizer grips.
                        if (!home.hittest(gear.coord))
                        {
                            auto center = area.coor + (area.size / 2);
                            gear.owner.SIGNAL(tier::release, e2::form::layout::shift, center);
                            boss.base::deface();
                        }
                    };
                    boss.LISTEN(tier::release, e2::form::proceed::detach, backup)
                    {
                        boss.mouse.reset();
                        boss.base::detach(); // The object kills itself.
                    };
                    boss.LISTEN(tier::release, e2::form::quit, nested_item)
                    {
                        boss.mouse.reset();
                        if (nested_item) boss.base::detach(); // The object kills itself.
                    };
                    boss.LISTEN(tier::release, e2::dtor, p)
                    {
                        auto start = datetime::now();
                        boss.SIGNAL(tier::general, e2::cleanup, counter, ());
                        auto stop = datetime::now() - start;
                        log("hall: garbage collection",
                        "\n\ttime ", utf::format(stop.count()), "ns",
                        "\n\tobjs ", counter.obj_count,
                        "\n\trefs ", counter.ref_count,
                        "\n\tdels ", counter.del_count);
                    };
                });
        }
        auto create(link& what)
        {
            SIGNAL(tier::request, vtm::events::newapp, what);
            auto slot = window(what);
            auto& cfg = dbase.menu[what.menuid];
            if (cfg.winsize && !what.forced) slot->extend({ what.square.coor, cfg.winsize });
            else                             slot->extend(what.square);
            slot->attach(what.applet);
            log("hall: app type: ", utf::debase(cfg.type), ", menu item id: ", utf::debase(what.menuid));
            this->branch(what.menuid, slot, !cfg.hidden);
            slot->SIGNAL(tier::anycast, e2::form::upon::started, this->This());
            return slot;
        }
        void nextframe(bool damaged) override
        {
            for (auto& u : users.items)
            {
                if (auto client = std::dynamic_pointer_cast<gate>(u->object))
                {
                    client->rebuild_scene(*this, damaged);
                }
            }
        }

    protected:
        hall(sptr<pipe> server, xmls& config, text defapp)
            : host{ server, config, pro::focus::mode::focusable }
        {
            auto current_module_file = os::process::binary();
            auto& apps_list = dbase.apps;
            auto& menu_list = dbase.menu;
            auto  free_list = std::list<std::pair<text, desk::spec>>{};
            auto  temp_list = free_list;
            auto  dflt_spec = desk::spec
            {
                .hidden   = faux,
                .slimmenu = faux,
                .type     = defapp,
            };
            auto find = [&](auto const& menuid) -> auto&
            {
                auto test = [&](auto& p) { return p.first == menuid; };

                auto iter_free = std::find_if(free_list.begin(), free_list.end(), test);
                if (iter_free != free_list.end()) return iter_free->second;

                auto iter_temp = std::find_if(temp_list.begin(), temp_list.end(), test);
                if (iter_temp != temp_list.end()) return iter_temp->second;

                return dflt_spec;
            };

            auto splitter_count = 0;
            for (auto item_ptr : host::config.list(path_item))
            {
                auto& item = *item_ptr;
                auto conf_rec = desk::spec{};
                //todo autogen id if absent
                conf_rec.splitter = item.take(attr_splitter, faux);
                conf_rec.menuid   = item.take(attr_id,       ""s );
                if (conf_rec.splitter)
                {
                    conf_rec.menuid = "splitter_" + std::to_string(splitter_count++);
                }
                else if (conf_rec.menuid.empty())
                {
                    log("hall: attribute '", utf::debase(attr_id), "' is missing, skip item");
                    continue;
                }
                auto label        = item.take(attr_label, ""s);
                conf_rec.label    = label.empty() ? conf_rec.menuid : label;
                conf_rec.alias    = item.take(attr_alias, ""s);
                auto& fallback = conf_rec.alias.empty() ? dflt_spec
                                                        : find(conf_rec.alias);
                conf_rec.hidden   = item.take(attr_hidden,   fallback.hidden  );
                conf_rec.notes    = item.take(attr_notes,    fallback.notes   );
                conf_rec.title    = item.take(attr_title,    fallback.title   );
                conf_rec.footer   = item.take(attr_footer,   fallback.footer  );
                conf_rec.bgc      = item.take(attr_bgc,      fallback.bgc     );
                conf_rec.fgc      = item.take(attr_fgc,      fallback.fgc     );
                conf_rec.winsize  = item.take(attr_winsize,  fallback.winsize );
                conf_rec.wincoor  = item.take(attr_wincoor,  fallback.wincoor );
                conf_rec.slimmenu = item.take(attr_slimmenu, fallback.slimmenu);
                conf_rec.hotkey   = item.take(attr_hotkey,   fallback.hotkey  ); //todo register hotkey
                conf_rec.cwd      = item.take(attr_cwd,      fallback.cwd     );
                conf_rec.param    = item.take(attr_param,    fallback.param   );
                conf_rec.type     = item.take(attr_type,     fallback.type    );
                auto patch        = item.list(attr_config);
                if (patch.size()) conf_rec.patch = patch.front()->snapshot();
                if (conf_rec.title.empty()) conf_rec.title = conf_rec.menuid + (conf_rec.param.empty() ? ""s : ": " + conf_rec.param);

                utf::to_low(conf_rec.type);
                utf::change(conf_rec.title,  "$0", current_module_file);
                utf::change(conf_rec.footer, "$0", current_module_file);
                utf::change(conf_rec.label,  "$0", current_module_file);
                utf::change(conf_rec.notes,  "$0", current_module_file);
                utf::change(conf_rec.param,  "$0", current_module_file);

                if (conf_rec.hidden) temp_list.emplace_back(std::move(conf_rec.menuid), std::move(conf_rec));
                else                 free_list.emplace_back(std::move(conf_rec.menuid), std::move(conf_rec));
            }
            for (auto& [menuid, conf_rec] : free_list)
            {
                apps_list[menuid];
                menu_list.emplace(std::move(menuid), std::move(conf_rec));
            }
            for (auto& [menuid, conf_rec] : temp_list)
            {
                menu_list.emplace(std::move(menuid), std::move(conf_rec));
            }

            LISTEN(tier::request, vtm::events::newapp, what)
            {
                auto& setup = dbase.menu[what.menuid];
                auto& maker = app::shared::builder(setup.type);
                what.applet = maker(setup.cwd, setup.param, host::config, setup.patch);
                what.header = setup.title;
                what.footer = setup.footer;
                if (setup.bgc     ) what.applet->SIGNAL(tier::anycast, e2::form::prop::colors::bg,   setup.bgc);
                if (setup.fgc     ) what.applet->SIGNAL(tier::anycast, e2::form::prop::colors::fg,   setup.fgc);
                if (setup.slimmenu) what.applet->SIGNAL(tier::anycast, e2::form::prop::ui::slimmenu, setup.slimmenu);
            };
            LISTEN(tier::general, e2::form::global::lucidity, alpha)
            {
                if (alpha == -1)
                {
                    alpha = skin::globals().lucidity;
                }
                else
                {
                    alpha = std::clamp(alpha, 0, 255);
                    skin::globals().lucidity = alpha;
                    this->SIGNAL(tier::preview, e2::form::global::lucidity, alpha);
                }
            };
            LISTEN(tier::preview, e2::form::proceed::detach, item_ptr)
            {
                auto& inst = *item_ptr;
                host::denote(items.remove(inst.id));
                auto block = users.remove(inst.id);
                if (block) // Save user's viewport last position.
                {
                    host::denote(block);
                    vport = block.coor;
                }
                if (dbase.remove(item_ptr))
                {
                    inst.SIGNAL(tier::release, e2::form::upon::vtree::detached, This());
                }
                if (items.size()) // Pass focus to the top most object.
                {
                    auto last_ptr = items.back();
                    item_ptr->RISEUP(tier::request, e2::form::state::keybd::enlist, gear_id_list, ());
                    for (auto gear_id : gear_id_list)
                    {
                        if (auto gear_ptr = bell::getref<hids>(gear_id))
                        {
                            auto& gear = *gear_ptr;
                            this->SIGNAL(tier::request, e2::form::state::keybd::next, gear_test, (gear_id, 0));
                            if (gear_test.second == 1) // If it is the last focused item.
                            {
                                pro::focus::set(last_ptr, gear.id, pro::focus::solo::off, pro::focus::flip::off);
                            }
                        }
                    }
                }
            };
            LISTEN(tier::release, e2::form::layout::bubble, inst)
            {
                auto region = items.bubble(inst.bell::id);
                host::denote(region);
            };
            LISTEN(tier::release, e2::form::layout::expose, inst)
            {
                auto region = items.expose(inst.bell::id);
                host::denote(region);
            };
            LISTEN(tier::request, desk::events::usrs, usrs_ptr)
            {
                usrs_ptr = dbase.usrs_ptr;
            };
            LISTEN(tier::request, desk::events::apps, apps_ptr)
            {
                apps_ptr = dbase.apps_ptr;
            };
            LISTEN(tier::request, desk::events::menu, menu_ptr)
            {
                menu_ptr = dbase.menu_ptr;
            };
            //todo unify
            LISTEN(tier::request, e2::form::layout::gonext, next)
            {
                if (items)
                if (auto next_ptr = items.rotate_next())
                {
                    next = next_ptr->object;
                }
            };
            LISTEN(tier::request, e2::form::layout::goprev, prev)
            {
                if (items)
                if (auto prev_ptr = items.rotate_prev())
                {
                    prev = prev_ptr->object;
                }
            };
            LISTEN(tier::request, e2::form::proceed::createby, gear)
            {
                static auto insts_count = si32{ 0 };
                auto& gate = gear.owner;
                auto location = gear.slot;
                if (gear.meta(hids::anyCtrl))
                {
                    log("hall: area copied to clipboard ", location);
                    gate.SIGNAL(tier::release, e2::command::printscreen, gear);
                }
                else
                {
                    auto what = link{ .square = gear.slot, .forced = gear.slot_forced };
                    gate.SIGNAL(tier::request, e2::data::changed, what.menuid);
                    if (auto window = create(what))
                    {
                        insts_count++;
                        window->LISTEN(tier::release, e2::form::upon::vtree::detached, master)
                        {
                            insts_count--;
                            log("hall: detached: ", insts_count);
                        };
                        pro::focus::set(window, gear.id, pro::focus::solo::on, pro::focus::flip::off);
                        window->SIGNAL(tier::anycast, e2::form::upon::created, gear); // Tile should change the menu item.
                    }
                }
            };
            LISTEN(tier::request, vtm::events::handoff, what)
            {
                auto& cfg = dbase.menu[what.menuid];
                auto slot = window(what);
                slot->extend(what.square);
                slot->attach(what.applet);
                log("hall: attach type=", utf::debase(cfg.type), " menuid=", utf::debase(what.menuid));
                this->branch(what.menuid, slot, !cfg.hidden);
                slot->SIGNAL(tier::anycast, e2::form::upon::started, this->This());
                what.applet = slot;
            };
            LISTEN(tier::release, hids::events::keybd::data::any, gear) // Last resort for unhandled kb event.
            {
                if (gear)
                {
                    gear.owner.SIGNAL(tier::release, hids::events::keybd::data::post, gear);
                }
            };
            LISTEN(tier::preview, hids::events::keybd::focus::cut, seed)
            {
                //todo revise: dtvt-app focus state can be wrong when user reconnects
                //if (seed.id == id_t{})
                //{
                //    this->SIGNAL(tier::release, hids::events::keybd::focus::bus::off, seed);
                //}
                //else
                if (auto gear_ptr = bell::getref<hids>(seed.id))
                {
                    auto& gear = *gear_ptr;
                    //seed.item = this->This();
                    gear.owner.SIGNAL(tier::preview, hids::events::keybd::focus::cut, seed);
                }
            };
            LISTEN(tier::preview, hids::events::keybd::focus::set, seed)
            {
                //todo revise
                //if (seed.id == id_t{})
                //{
                //    this->SIGNAL(tier::release, hids::events::keybd::focus::bus::on, seed);
                //}
                //else
                if (auto gear_ptr = bell::getref<hids>(seed.id))
                {
                    auto& gear = *gear_ptr;
                    seed.item = this->This();
                    gear.owner.SIGNAL(tier::preview, hids::events::keybd::focus::set, seed);
                }
            };
        }

    public:
       ~hall()
        {
            auto lock = netxs::events::sync{};
            dbase.reset();
            items.reset();
        }

        // hall: Autorun apps from config.
        void autorun()
        {
            vport = host::config.take(path_viewport, dot_00);
            auto what = link{};
            auto apps = host::config.list(path_autorun);
            auto foci = std::vector<sptr<base>>();
            foci.reserve(apps.size());
            for (auto app_ptr : apps)
            {
                auto& app = *app_ptr;
                if (!app.fake)
                {
                    what.menuid =   app.take(attr_id, ""s);
                    what.square = { app.take(attr_wincoor, dot_00),
                                    app.take(attr_winsize, twod{ 80,25 }) };
                    auto focused =  app.take(attr_focused, faux);
                    what.forced = !!what.square.size;
                    if (what.menuid.size())
                    {
                        auto window_ptr = create(what);
                        if (focused) foci.push_back(window_ptr);
                    }
                    else log("hall: Unexpected empty app id in autorun configuration");
                }
            }
            auto count = 0;
            for (auto& window_ptr : foci)
            {
                pro::focus::set(window_ptr, id_t{}, count++ ? pro::focus::solo::off // Reset all foci on the first item.
                                                            : pro::focus::solo::on, pro::focus::flip::off);
            }
            if constexpr (debugmode)
            {
                SIGNAL(tier::request, e2::form::state::keybd::next, gear_test, (0,0));
                log("hall: autofocused items count:", gear_test.second);
            }
        }
        void redraw(face& canvas) override
        {
            users.prerender (canvas); // Draw backpane for spectators.
            items.render    (canvas); // Draw objects of the world.
            users.postrender(canvas); // Draw spectator's mouse pointers.
        }
        // hall: Attach a new item to the scene.
        template<class S>
        void branch(text const& menuid, sptr<S> item, bool fixed = true)
        {
            items.append(item);
            item->base::root(true); //todo move it to the window creator (main)
            auto& [stat, list] = dbase.apps[menuid];
            stat = fixed;
            list.push_back(item);
            item->SIGNAL(tier::release, e2::form::upon::vtree::attached, base::This());
            item->SIGNAL(tier::anycast, vtm::events::attached, base::This());
            this->SIGNAL(tier::release, desk::events::apps, dbase.apps_ptr);
        }
        // hall: Create a new user gate.
        auto invite(sptr<pipe> client, view userid, si32 vtmode, xmls config, si32 session_id)
        {
            auto lock = netxs::events::unique_lock();
            auto user = base::create<gate>(client, userid, vtmode, config, session_id);
            users.append(user);
            dbase.append(user);
            user->SIGNAL(tier::release, e2::form::upon::vtree::attached, base::This());
            this->SIGNAL(tier::release, desk::events::usrs, dbase.usrs_ptr);
            //todo make it configurable
            auto patch = ""s;
            auto deskmenu = app::shared::builder(app::desk::id)("", utf::concat(user->id, ";", user->props.os_user_id, ";", user->props.selected), config, patch);
            user->attach(deskmenu);
            if (vport) user->base::moveto(vport); // Restore user's last position.
            lock.unlock();
            user->launch();
        }
    };
}