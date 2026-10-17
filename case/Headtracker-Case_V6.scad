// ============================================================================
// FPV Headtracker Gehäuse v6: Waveshare ESP32-C3 Mini (Zero) + Adafruit BNO085
// Neu in v6:
// - Lichttunnel-Zylinder innen entfernt (keine Kollision mehr mit USB-C-Buchse,
//   nur noch flaches 2.0-mm-Sichtloch im Deckel)
// - Stößellänge für BOOT & RESET direkt auf gemessene 1.7 mm eingestellt
// - Integrierter Schnapp-Clip am hinteren Ende (+X) des ESP32-C3: hält die
//   Platine gegen hochdrückende Kabel unten, bis der L-Deckel geschlossen wird
// Voraussetzung: BOSL2 Library (https://github.com/BelfrySCAD/BOSL2)
// ============================================================================

include <BOSL2/std.scad>

/* [Layout & Ansicht] */
// Bauform: "stacked" (kompakter Turm mit L-Deckel) oder "flat" (flach nebeneinander)
layout_mode      = "stacked";   // ["stacked", "flat"]
// Stapel-Reihenfolge im Stacked-Modus ("c3_top" empfohlen für L-Deckel, Taster & LED)
stack_order      = "c3_top";    // ["c3_top", "c3_bottom"]
// Welche Teile sollen gerendert werden? ("both" = druckfertig auf Z=0, "assembled" = 3D-Vorschau)
part_select      = "both";      // ["both", "base", "lid", "assembled"]
// Explosionsansicht für die Vorschau
explode_z        = 0;           // [0:1:35]

/* [L-Deckel, Einschub & C3-Clip (v6)] */
// Rückwand (+X) als Teil des Deckels für seitlichen/hinteren BNO085-Einschub
rear_wall_lid    = true;
// Hinterer Montage-Schnappclip für den ESP32-C3 (hält ihn gegen Kabeldruck unten)
enable_c3_clip   = true;
// Schnapp-Überhang des C3-Clips in mm (0.45 mm schnappt leicht ein und hält sicher)
c3_clip_overhang = 0.45;        // [0.30:0.05:0.70]
// Vertikales Spiel unter dem C3-Clip in mm
c3_clip_z_tol    = 0.25;        // [0.10:0.05:0.60]
// Vertikaler Abstand zwischen BNO085-Auflage und ESP32-C3-Auflage
deck_clearance   = 8.4;         // [6.4:0.2:12.0]
// Vertikales Höhenspiel für den BNO085 im seitlichen Führungsschlitz (PETG-freundlich)
bno_z_tol        = 0.45;        // [0.20:0.05:0.80]
// Auflagebreite der unteren BNO085-Schiene am äußeren Gehäuserand (lässt Lötpads frei)
bno_edge_ledge_w = 1.25;        // [1.0:0.05:1.6]
// Durchgehende Auflagebreite pro Seite für den ESP32-C3
c3_shelf_w       = 1.10;        // [0.8:0.05:1.5]
// Kopfraum über dem ESP32-C3 (4.4 mm ergibt ~1.2 mm stabilen Steg über USB-C)
c3_top_clearance = 4.4;         // [3.6:0.1:5.5]

/* [Taster (BOOT/RESET) & RGB-LED Optionen] */
// Integrierte Federzungen-Drücker im Deckel aktivieren
enable_buttons   = true;
// Direkte Stößellänge ab Deckel-Innenseite (Oberkante Unterteil) in mm
plunger_len      = 1.70;        // [1.00:0.05:2.50]
// LED-Fenster-Typ: "lightpipe" (offenes 2.0mm Sichtloch), "diffuser" (0.4mm geschlossen), "none"
led_window_type  = "lightpipe"; // ["lightpipe", "diffuser", "none"]
// Positionen auf dem Waveshare ESP32-C3-Zero (gemessen ab Vorderkante bei USB-C = X:0, Mitte = Y:0)
boot_pos_x       = 10.8;        // Abstand BOOT-Taster von vorderer Platinenkante (USB-C Seite)
boot_pos_y       = 4.3;         // Seitlicher Versatz BOOT-Taster (+Y Seite neben USB-C)
rst_pos_x        = 10.8;        // Abstand RESET-Taster von vorderer Platinenkante
rst_pos_y        = -4.3;        // Seitlicher Versatz RESET-Taster (-Y Seite neben USB-C)
led_pos_x        = 7.7;         // Abstand WS2812 RGB-LED von vorderer Platinenkante
led_pos_y        = 0.0;         // Seitlicher Versatz WS2812 RGB-LED

/* [Befestigung am Brillenband] */
mount_type       = "velcro_flat"; // ["strap", "velcro_flat"]
strap_width      = 22.0;        // Breite des Brillenbands in mm
strap_thick      = 2.2;         // Durchführschlitz-Breite in mm

/* [Druck-Toleranzen & Wandstärken] */
wall             = 1.8;         // Wandstärke Gehäuse
floor_h          = 1.6;         // Boden- und Deckelstärke
pcb_tol          = 0.40;        // Seitliches Platinenspiel (0.20 mm pro Seite)
snap_tol         = 0.15;        // Spaltmaß für den Schnappdeckel

/* [Platinen-Maße: Waveshare ESP32-C3 Mini / Zero] */
c3_len           = 23.8;
c3_wid           = 18.0;
c3_pcb_h         = 1.7;
usbc_w           = 9.4;         // USB-C Durchbruch Breite
usbc_h           = 3.7;         // USB-C Durchbruch Höhe
usbc_z_off       = 1.8;         // Offset USB-C relativ zur Platinen-Unterseite

/* [Platinen-Maße: Adafruit BNO085 STEMMA QT (4754)] */
bno_len          = 25.6;
bno_wid          = 22.8;
bno_pcb_h        = 1.70;        // Reale Platinendicke inkl. Lötstopplack
bno_hole_x       = 20.32;       // 0.80 inch Lochabstand X
bno_hole_y       = 17.78;       // 0.70 inch Lochabstand Y
bno_hole_d       = 2.2;

$fn = 48;

// --- Berechnete Gehäusemaße & Einbauhöhen ---
use_l_lid    = rear_wall_lid && (layout_mode == "stacked") && (stack_order == "c3_top");

gap_between  = 2.5;
c3_bay_x     = c3_len + pcb_tol;
c3_bay_y     = c3_wid + pcb_tol;
bno_bay_x    = bno_len + pcb_tol;
bno_bay_y    = bno_wid + pcb_tol;

base_ledge_h = 1.8;                           // 1.8 mm tiefer Freiraum unter den BNO085-Lötpunkten
upper_deck_z = base_ledge_h + deck_clearance; // Einbauhöhe der oberen Platine (Standard: 10.2 mm)

// Aktuelle Z-Position (Unterkante Platine relativ zum Innenboden) für C3 und BNO085
c3_z_pos     = (layout_mode == "stacked" && stack_order == "c3_top") ? upper_deck_z : base_ledge_h;
bno_z_pos    = (layout_mode == "stacked" && stack_order == "c3_bottom") ? upper_deck_z : base_ledge_h;

inner_x      = (layout_mode == "stacked") ? bno_bay_x + 1.2 : (c3_bay_x + bno_bay_x + gap_between + 1.0);
// Innenbreite exakt auf bno_bay_y abgestimmt, damit die Platine seitlich direkt an der Wand geführt wird
inner_y      = bno_bay_y + 0.2;
inner_z      = (layout_mode == "stacked") ? (upper_deck_z + c3_pcb_h + c3_top_clearance) : 8.5;

outer_x      = inner_x + 2 * wall;
outer_y      = inner_y + 2 * wall;
outer_z      = floor_h + inner_z;

// X-Positionen von ESP32-C3 (-X bündig) und BNO085
c3_front_x   = -inner_x/2;
c3_center_x  = c3_front_x + c3_bay_x/2;
c3_rear_x    = c3_front_x + c3_bay_x;         // Hinterkante des ESP32-C3 (+10.6 mm)
bno_front_x  = -inner_x/2 + 0.4;              // 0.4 mm vorderer Anschlagsteg

// Maße für die vertikale Nut-und-Feder-Führung des L-Deckels
flange_len_x = 2.0;                           // Tiefe der hinteren Seitenwangen in X
tongue_x     = 1.0;                           // Breite der vertikalen Feder in X
tongue_y     = 0.55;                          // Eingriffstiefe der Feder in die Seitenwand

// ============================================================================
// Haupt-Steuerung
// ============================================================================
if (part_select == "both") {
    base_housing();
    translate([outer_x + 10, 0, 0])
        lid_housing();
} else if (part_select == "base") {
    base_housing();
} else if (part_select == "lid") {
    lid_housing();
} else if (part_select == "assembled") {
    base_housing();
    translate([explode_z * 0.35, 0, outer_z + floor_h + explode_z])
        xrot(180)
            lid_housing();
}

// ============================================================================
// Modul: Unterteil (Base)
// ============================================================================
module base_housing() {
    difference() {
        union() {
            // Grundkörper
            cuboid([outer_x, outer_y, outer_z], rounding=2.5, edges="Z", anchor=BOTTOM);
            
            // Optional: Brillenband-Laschen
            if (mount_type == "strap") {
                strap_loops();
            }
        }
        
        // 1. Innenraum aushöhlen
        // Vorne (-X) kleiner Innenradius (0.5 mm), damit die äußere 1.25-mm-Randauflage volle Breite hat
        up(floor_h)
            cuboid([inner_x, inner_y, inner_z + 1.0],
                   rounding=0.5,
                   edges=use_l_lid ? [FRONT+LEFT, BACK+LEFT] : "Z",
                   anchor=BOTTOM);
                   
        // 2. Bei aktivem L-Deckel: Rückwand (+X) oberhalb der Bodenplatte komplett öffnen
        if (use_l_lid) {
            translate([inner_x/2 - snap_tol, 0, floor_h])
                cuboid([wall + 2.0, outer_y + 2.0, inner_z + 2.0], anchor=BOTTOM+LEFT);
                
            translate([0, 0, floor_h])
                cuboid([outer_x/2 + 2.0, inner_y, inner_z + 2.0], anchor=BOTTOM+LEFT);
                
            // Vertikale Führungsnut (Nut & Feder) in den hinteren Seitenwänden (+/-Y)
            groove_z_start = floor_h + bno_z_pos + bno_pcb_h + bno_z_tol;
            for (side = [-1, 1]) {
                translate([inner_x/2 - flange_len_x/2, side * (inner_y/2), groove_z_start])
                    cuboid([tongue_x + 2*snap_tol, 2*(tongue_y + snap_tol), outer_z - groove_z_start + 1.0],
                           anchor=BOTTOM);
            }
        }
            
        // 3. USB-C Aussparung an der linken Stirnseite (-X)
        up(floor_h + c3_z_pos + usbc_z_off + usbc_h/2 - 0.5)
            left(outer_x/2)
                cuboid([wall * 3, usbc_w, usbc_h], rounding=1.2, edges="X");
                
        // 4. Obere Einrastrille für Schnappdeckel an den Längswänden (Y)
        up(outer_z - 1.2)
            for (side = [-1, 1])
                translate([0, side * (inner_y/2 + 0.3), 0])
                    xcyl(l=inner_x - 8, d=1.1, $fn=24);
    }
    
    // Innen-Aufnahmen je nach gewähltem Layout
    up(floor_h) {
        if (layout_mode == "stacked") {
            if (stack_order == "c3_top") {
                stacked_c3_top_internals();
            } else {
                stacked_c3_bottom_internals();
            }
        } else {
            flat_internals();
        }
    }
}

// ============================================================================
// Innenleben v6: BNO085-Randauflage + durchgehende C3-Schiene + C3-Schnappclip
// ============================================================================
module stacked_c3_top_internals() {
    // ------------------------------------------------------------------------
    // 1. UNTERES DECK: BNO085-Einschub (komplett ohne Rastnocken, Lötkanal frei)
    // ------------------------------------------------------------------------
    rail_len_bno = inner_x;
    bno_upper_z  = bno_z_pos + bno_pcb_h + bno_z_tol; // Standard: 1.8 + 1.7 + 0.45 = 3.95 mm
    upper_over_y = 1.15;                              // Greift nur 1.15 mm über den äußersten Rand
    
    // A) Vorderer X-Anschlagsteg an der -X Stirnwand (nur außen an den Ecken)
    for (side = [-1, 1]) {
        translate([-inner_x/2 + 0.2, side * (inner_y/2 - 2.0), 0])
            cuboid([0.4, 4.0, bno_upper_z + 0.4], anchor=BOTTOM);
    }
    
    // B) Untere Rand-Auflageschienen DIREKT an den Seitenwänden (+/-Y = inner_y/2)
    for (side = [-1, 1]) {
        translate([0, side * (inner_y/2 - bno_edge_ledge_w/2), 0])
            cuboid([rail_len_bno, bno_edge_ledge_w, bno_z_pos],
                   chamfer=0.35, edges=[TOP+RIGHT], anchor=BOTTOM);
    }
    
    // C) Schmale Mittel-Auflageleiste bei Y = 0 (Breite 1.8 mm)
    cuboid([rail_len_bno, 1.8, bno_z_pos],
           chamfer=0.35, edges=[TOP+RIGHT, TOP+LEFT], anchor=BOTTOM);
           
    // D) Obere BNO085-Führungsleisten direkt an der Seitenwand (mit Einführschräge hinten)
    guide_len_x = inner_x - flange_len_x - 0.3;
    for (side = [-1, 1]) {
        translate([-inner_x/2 + guide_len_x/2, side * (inner_y/2), bno_upper_z])
            bno_edge_upper_guide(guide_len_x, upper_over_y, side);
    }

    // ------------------------------------------------------------------------
    // 2. OBERES DECK: Durchgehende 45°-Schienen für ESP32-C3 + hinterer Clip
    // ------------------------------------------------------------------------
    bord_w        = (inner_y - c3_bay_y) / 2;   // Seitliche Führungswand-Dicke (~2.5 mm)
    rail_overhang = bord_w + c3_shelf_w;        // Gesamt-Überhang inkl. 1.1 mm Auflage (~3.6 mm)
    rail_len      = c3_bay_x + 1.0;             // Reicht 1.0 mm über das C3-Ende für den Clip-Träger
    
    for (side = [-1, 1]) {
        translate([c3_front_x + rail_len/2, side * (inner_y/2), c3_z_pos]) {
            // Durchgehende 45°-Stützrampe unterhalb der gesamten C3-Schiene
            hull() {
                translate([0, -side * (rail_overhang/2), 0])
                    cuboid([rail_len, rail_overhang, 0.15], anchor=TOP);
                translate([0, -side * 0.1, -rail_overhang])
                    cuboid([rail_len, 0.2, 0.1], anchor=TOP);
            }
            // Seitliche Führungswand neben der C3-Platine (fixiert Y-Richtung)
            translate([0, -side * (bord_w/2), 0])
                cuboid([rail_len, bord_w, c3_pcb_h + 0.6],
                       chamfer=0.35, edges=[TOP+LEFT, TOP+RIGHT], anchor=BOTTOM);
        }
    }
    
    // ------------------------------------------------------------------------
    // 3. HINTERER SCHNAPP-CLIP (+X Ende des ESP32-C3)
    //    Hält den C3 gegen den Druck der Kabel unten, bis der Deckel sitzt.
    //    Sitzt bei Y = +/-4.5 mm (Eck-Lötpads bei Y=+/-7.8 und Mitte bleiben frei!)
    // ------------------------------------------------------------------------
    if (enable_c3_clip) {
        clip_thick_x = 1.0;                        // 1.0 mm federnder Steg (dahinter freier Raum!)
        clip_w_y     = 2.4;                        // Breite je Rastfinger in Y
        clip_pos_y   = 4.5;                        // Y-Position (innerhalb der Eck-Lötpads)
        clip_hook_z  = c3_z_pos + c3_pcb_h + c3_clip_z_tol;
        
        for (side = [-1, 1]) {
            // A) Supportfreier 45°-Trägerarm UNTERHALB der Platinen-Oberkante von der
            //    Seitenwand nach innen bis Y = +/-3.3 mm (lässt Eck-Lötpads oben komplett frei!)
            arm_y_outer = inner_y/2;
            arm_y_inner = clip_pos_y - clip_w_y/2;
            arm_span_y  = arm_y_outer - arm_y_inner;
            
            translate([c3_rear_x + clip_thick_x/2, side * (arm_y_inner + arm_span_y/2), c3_z_pos]) {
                hull() {
                    cuboid([clip_thick_x, arm_span_y, 0.15], anchor=TOP);
                    translate([0, side * (arm_span_y/2 - 0.2), -arm_span_y * 0.85 + 0.8])
                        cuboid([clip_thick_x, 0.4, 0.1], anchor=TOP);
                }
            }
            
            // B) Federnder vertikaler Rastfinger bei Y = +/-4.5 mm
            translate([c3_rear_x + clip_thick_x/2, side * clip_pos_y, c3_z_pos - 0.8])
                cuboid([clip_thick_x, clip_w_y, (clip_hook_z - c3_z_pos) + 0.8 + 0.9], anchor=BOTTOM);
                
            // C) Schnapp-Nase (ragt um c3_clip_overhang = 0.45 mm nach -X über die C3-Hinterkante)
            //    Unten 45° für supportfreien Druck, oben 45° Einführschräge zum einfachen Einklipsen
            translate([c3_rear_x, side * clip_pos_y, clip_hook_z]) {
                hull() {
                    // Hintere Basis am Rastfinger
                    translate([clip_thick_x/2, 0, -c3_clip_overhang])
                        cuboid([clip_thick_x, clip_w_y, c3_clip_overhang + 0.9], anchor=BOTTOM);
                    // Vorderste Schnappkante (ragt nach -X über die Platine)
                    translate([-c3_clip_overhang/2, 0, 0])
                        cuboid([c3_clip_overhang, clip_w_y, 0.25], anchor=BOTTOM);
                }
            }
        }
    } else if (!use_l_lid) {
        // Klassischer starrer Anschlagsteg, falls Clip und L-Deckel deaktiviert sind
        translate([c3_rear_x + 0.6, 0, c3_z_pos]) {
            for (side = [-1, 1]) {
                translate([0, side * (c3_bay_y/2 - 1.5), 0]) {
                    hull() {
                        cuboid([1.2, 3.6, c3_pcb_h + 0.6], anchor=BOTTOM);
                        translate([0, side * 2.0, -3.0])
                            cuboid([1.2, 1.0, 0.1], anchor=BOTTOM);
                    }
                }
            }
        }
    }
}

// Hilfsmodul: Obere Randführung für den BNO085 direkt an der Seitenwand
// (mit sanfter Einführschräge am hinteren +X-Ende für leichtes Einfädeln)
module bno_edge_upper_guide(len_x, over_y, side) {
    difference() {
        hull() {
            translate([0, -side * (over_y/2), 0])
                cuboid([len_x, over_y, 0.8], anchor=BOTTOM);
            translate([0, -side * 0.1, 0.8 + over_y])
                cuboid([len_x, 0.2, 0.1], anchor=BOTTOM);
        }
        translate([len_x/2 + 0.1, -side * (over_y/2), -0.1])
            yrot(-20)
                cuboid([2.5, over_y + 1.0, 1.2], anchor=BOTTOM+RIGHT);
    }
}

// ============================================================================
// Innenleben: Stacked mit C3 UNTEN (Klassisch)
// ============================================================================
module stacked_c3_bottom_internals() {
    translate([c3_center_x, 0, 0]) {
        difference() {
            cuboid([c3_bay_x + 2.0, c3_bay_y + 2.4, c3_z_pos + c3_pcb_h], anchor=BOTTOM);
            up(c3_z_pos)
                cuboid([c3_bay_x, c3_bay_y, c3_pcb_h + 1], anchor=BOTTOM);
            cuboid([c3_bay_x - 2.2, c3_bay_y - 2.0, c3_z_pos + 1], anchor=BOTTOM);
            left(c3_bay_x/2)
                cuboid([4, c3_bay_y - 2, c3_z_pos + 1 + c3_pcb_h], anchor=BOTTOM);
        }
    }
    for (dx = [-1, 1], dy = [-1, 1]) {
        hx = dx * (bno_hole_x / 2);
        hy = dy * (bno_hole_y / 2);
        translate([hx, hy, bno_z_pos]) {
            difference() {
                hull() {
                    cyl(h=0.1, d=4.6, anchor=TOP);
                    translate([dx * 1.2, dy * 2.0, -4.2])
                        cuboid([2.4, 2.0, 0.1], anchor=TOP);
                }
                down(4.5)
                    cyl(h=5.0, d=1.75, anchor=BOTTOM);
            }
            cyl(h=1.4, d=bno_hole_d - 0.15, anchor=BOTTOM);
        }
    }
}

// ============================================================================
// Innenleben: Flat (Nebeneinander)
// ============================================================================
module flat_internals() {
    translate([c3_center_x, 0, 0]) {
        difference() {
            cuboid([c3_bay_x + 1.8, c3_bay_y + 2.4, c3_z_pos + c3_pcb_h], anchor=BOTTOM);
            up(c3_z_pos)
                cuboid([c3_bay_x, c3_bay_y, c3_pcb_h + 1], anchor=BOTTOM);
            cuboid([c3_bay_x - 2.2, c3_bay_y - 2.0, c3_z_pos + 1], anchor=BOTTOM);
            left(c3_bay_x/2)
                cuboid([4, c3_bay_y - 2, c3_z_pos + c3_pcb_h + 1], anchor=BOTTOM);
        }
    }
    bno_x_pos = inner_x/2 - bno_bay_x/2 - 0.5;
    translate([bno_x_pos, 0, 0]) {
        for (dx = [-1, 1], dy = [-1, 1]) {
            translate([dx * bno_hole_x/2, dy * bno_hole_y/2, 0]) {
                difference() {
                    cyl(h=bno_z_pos, d=4.6, anchor=BOTTOM);
                    cyl(h=bno_z_pos + 0.5, d=1.75, anchor=BOTTOM);
                }
                up(bno_z_pos)
                    cyl(h=1.4, d=bno_hole_d - 0.15, anchor=BOTTOM);
            }
        }
    }
}

// ============================================================================
// Modul: Brillenband-Laschen
// ============================================================================
module strap_loops() {
    loop_w   = strap_width + 4.0;
    loop_ext = strap_thick + 2.2;
    for (side = [-1, 1]) {
        translate([0, side * (outer_y/2 + loop_ext/2 - 0.5), 0]) {
            difference() {
                cuboid([min(outer_x, loop_w), loop_ext + 1.0, floor_h + 1.2],
                       rounding=1.5, edges="Z", anchor=BOTTOM);
                translate([0, side * 0.2, -0.5])
                    cuboid([strap_width, strap_thick, floor_h + 3.0],
                           rounding=0.6, edges="Z", anchor=BOTTOM);
            }
        }
    }
}

// ============================================================================
// Modul: Schnapp-Deckel (Lid) inkl. L-Rückwand, 1.7mm Stößeln & LED-Loch
// Hinweis: Der Deckel liegt mit der Außenseite auf Z=0 (direkt druckfertig!).
// ============================================================================
module lid_housing() {
    lip_h = 2.0;
    lip_x = inner_x - 2 * snap_tol;
    lip_y = inner_y - 2 * snap_tol;
    
    // Prüfen, ob der ESP32-C3 frei unter dem Deckel liegt
    c3_under_lid = (layout_mode == "flat") || (stack_order == "c3_top");
    
    // Koordinaten im Deckel (Y gespiegelt für korrekte Deckel-Montage)
    boot_lid_x = c3_front_x + pcb_tol/2 + boot_pos_x;
    boot_lid_y = -boot_pos_y;
    rst_lid_x  = c3_front_x + pcb_tol/2 + rst_pos_x;
    rst_lid_y  = -rst_pos_y;
    led_lid_x  = c3_front_x + pcb_tol/2 + led_pos_x;
    led_lid_y  = -led_pos_y;

    // Gesamthöhe der integrierten L-Rückwand (reicht bis auf die Bodenplatte des Unterteils)
    rear_wall_h = floor_h + inner_z - snap_tol;
    
    // Niederhalter-Höhe für den C3 (abgestimmt auf plunger_len + SMD-Tasterhöhe,
    // damit der Deckel sauber schließt, auch wenn der C3 im Clip anliegt)
    c3_hold_h   = max(1.5, min(inner_z - (c3_z_pos + c3_pcb_h + c3_clip_z_tol + 0.15), plunger_len + 1.1));

    difference() {
        union() {
            // 1. Deckelplatte (liegt plan auf dem Druckbett bei Z=0)
            cuboid([outer_x, outer_y, floor_h], rounding=2.5, edges="Z", anchor=BOTTOM);
            
            // 2. Integrierte L-Rückwand (+X) mit Nut-und-Feder-Seitenwangen & Platinen-Klemmung
            if (use_l_lid) {
                // A) Äußere Rückwand mit exakt passender 2.5mm Eckrundung
                intersection() {
                    cuboid([outer_x, outer_y, rear_wall_h], rounding=2.5, edges="Z", anchor=BOTTOM);
                    translate([inner_x/2, 0, 0])
                        cuboid([wall + 1.0, outer_y + 1.0, rear_wall_h + 1.0], anchor=BOTTOM+LEFT);
                }
                
                // B) Vertikale Seitenwangen mit Nut-und-Feder-Steg
                flange_h = inner_z - (bno_z_pos + bno_pcb_h + bno_z_tol + 0.1);
                for (side = [-1, 1]) {
                    translate([inner_x/2 - flange_len_x/2, side * (lip_y/2 - 0.55), floor_h])
                        cuboid([flange_len_x, 1.1, flange_h], anchor=BOTTOM);
                        
                    // Vertikale Feder (greift in die Nut der hinteren Gehäuse-Seitenwand)
                    translate([inner_x/2 - flange_len_x/2, side * (lip_y/2), floor_h])
                        cuboid([tongue_x, 2 * tongue_y, flange_h - 0.4],
                               chamfer=0.3, edges="Z", anchor=BOTTOM);
                }
                
                // C) ESP32-C3 Niederhalter (mittig bei X = c3_front_x + 15.0 platziert)
                for (side = [-1, 1]) {
                    translate([c3_front_x + 15.0, side * (c3_bay_y/2 - 0.6), floor_h])
                        cuboid([3.6, 1.2, c3_hold_h], anchor=BOTTOM);
                }
                
                // D) Hinterer Stütz-Anschlag hinter den C3-Schnappclips (lässt 1.25 mm Platz
                //    für die 1.0 mm dünnen Rastfinger des Unterteils und stützt diese im
                //    geschlossenen Zustand von hinten ab!)
                clip_clearance_x = enable_c3_clip ? 1.25 : 0.20;
                c3_stop_len      = max(0.4, inner_x/2 - (c3_rear_x + clip_clearance_x));
                c3_stop_h        = inner_z - c3_z_pos + 0.4;
                for (side = [-1, 1]) {
                    translate([inner_x/2 - c3_stop_len/2, side * 4.5, floor_h])
                        cuboid([c3_stop_len, 2.2, c3_stop_h], anchor=BOTTOM);
                }
                
                // E) Hinterer X-Anschlag für den BNO085 (unten an der Innenseite der Rückwand)
                bno_rear_edge = bno_front_x + bno_bay_x;
                bno_stop_len  = max(0.4, inner_x/2 - (bno_rear_edge + 0.2));
                bno_stop_z1   = floor_h + flange_h - 0.4;
                bno_stop_z2   = rear_wall_h - 0.3;
                
                for (side = [-1, 1]) {
                    translate([inner_x/2 - bno_stop_len/2, side * (bno_hole_y/2), bno_stop_z1])
                        hull() {
                            cuboid([bno_stop_len, 3.2, bno_stop_z2 - bno_stop_z1], anchor=BOTTOM);
                            translate([bno_stop_len/2 - 0.05, 0, -0.8])
                                cuboid([0.1, 3.2, 0.1], anchor=BOTTOM);
                        }
                }
            }
            
            // 3. Umlaufende Zentrier-Lippe (mit Freischnitt vor den Federzungen)
            up(floor_h)
                difference() {
                    cuboid([lip_x, lip_y, lip_h], rounding=0.8, edges="Z", anchor=BOTTOM);
                    cuboid([lip_x - 2.4, lip_y - 2.4, lip_h + 1], anchor=BOTTOM);
                    if (enable_buttons && c3_under_lid) {
                        left(lip_x/2)
                            cuboid([6.0, 16.0, lip_h + 1], anchor=BOTTOM);
                    }
                }
                
            // 4. Schnapp-Wulst an den Längsseiten
            up(floor_h + 1.2)
                for (side = [-1, 1])
                    translate([0, side * (lip_y/2), 0])
                        xcyl(l=inner_x - 10, d=0.9, $fn=24);
                        
            // 5. Integrierte Stößel für BOOT & RESET (exakt plunger_len = 1.7 mm lang)
            if (enable_buttons && c3_under_lid && plunger_len > 0.3) {
                for (pt = [[boot_lid_x, boot_lid_y], [rst_lid_x, rst_lid_y]]) {
                    translate([pt[0], pt[1], floor_h]) {
                        cyl(h=min(0.8, plunger_len), d1=2.6, d2=2.2, anchor=BOTTOM);
                        cyl(h=plunger_len, d=2.2, anchor=BOTTOM);
                    }
                }
            }
            
            // (Hinweis: Der nach innen stehende Lichttunnel-Zylinder wurde in v6 entfernt,
            //  damit über der USB-C-Buchse maximaler Freiraum bleibt!)
            
            // 6. Niederhalter für die obere Platine (falls kein L-Deckel aktiv ist)
            if (layout_mode == "flat" || stack_order == "c3_bottom") {
                bno_x_lid   = (layout_mode == "stacked") ? 0 : (inner_x/2 - bno_bay_x/2 - 0.5);
                hold_down_h = inner_z - bno_z_pos - bno_pcb_h - 0.15;
                translate([bno_x_lid, 0, floor_h]) {
                    for (dx = [-1, 1], dy = [-1, 1]) {
                        translate([dx * bno_hole_x/2, dy * bno_hole_y/2, 0])
                            difference() {
                                cyl(h=hold_down_h, d=4.2, anchor=BOTTOM);
                                cyl(h=hold_down_h + 0.5, d=bno_hole_d + 0.3, anchor=BOTTOM);
                            }
                    }
                }
            } else if (!use_l_lid) {
                for (side = [-1, 1]) {
                    translate([c3_front_x + 14.0, side * (c3_bay_y/2 - 0.8), floor_h])
                        cuboid([4.0, 1.6, c3_hold_h], anchor=BOTTOM);
                }
            }
        }
        
        // --- Abzüge im Deckel (Federzungen-Schlitze, flaches LED-Loch, Lüftung, Griffkerbe) ---
        
        // A) U-förmige Freischnitte für die BOOT- und RESET-Federzungen
        if (enable_buttons && c3_under_lid) {
            for (btn = [[boot_lid_x, boot_lid_y], [rst_lid_x, rst_lid_y]]) {
                bx = btn[0];
                by = btn[1];
                
                tongue_w = 3.2;
                tongue_l = 5.5;
                slot_w   = 0.55;
                
                translate([bx, by, -0.5]) {
                    left(1.6)
                        cuboid([slot_w, tongue_w + 2*slot_w, floor_h + 1.5], anchor=BOTTOM);
                    for (s = [-1, 1])
                        translate([tongue_l/2 - 1.6, s * (tongue_w/2 + slot_w/2), 0])
                            cuboid([tongue_l + slot_w, slot_w, floor_h + 1.5], anchor=BOTTOM);
                }
                
                // Biegegelenk-Entlastung innen an der Zungenwurzel
                translate([bx + tongue_l - 2.0, by, floor_h - 0.6])
                    cuboid([1.6, tongue_w + 0.2, 1.0], anchor=BOTTOM);
            }
        }
        
        // B) Einfaches LED-Sichtloch im Deckel (ohne inneren Lichttunnel)
        if (led_window_type != "none" && c3_under_lid) {
            z_start = (led_window_type == "diffuser") ? 0.4 : -0.5;
            translate([led_lid_x, led_lid_y, z_start])
                cyl(h=floor_h + 1.0, d=2.0, anchor=BOTTOM);
                
            if (led_window_type == "lightpipe") {
                translate([led_lid_x, led_lid_y, -0.05])
                    cyl(h=0.5, d1=2.8, d2=2.0, anchor=BOTTOM);
            }
        }
        
        // C) Lüftungsschlitze
        vent_start_x = (layout_mode == "stacked") ? 3.8 : 4.0;
        for (i = [0:2]) {
            translate([vent_start_x + i * 3.2, 0, -0.5])
                cuboid([1.4, inner_y * 0.48, floor_h + 2], rounding=0.5, edges="Z", anchor=BOTTOM);
        }
        
        // D) Fingernagel-Griffkerbe unten an der L-Rückwand zum Öffnen
        if (use_l_lid) {
            translate([outer_x/2 + 0.1, 0, rear_wall_h + 0.1])
                cuboid([1.4, 7.0, 0.9], chamfer=0.4, edges="Y", anchor=TOP+RIGHT);
        }
    }
}