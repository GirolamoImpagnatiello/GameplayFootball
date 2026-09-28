#!/usr/bin/env python3
"""Install the 2026/27 big-five league clubs and UV-safe recognisable kits."""

from __future__ import annotations

import random
import shutil
import sqlite3
import colorsys
import sys
from dataclasses import dataclass
from pathlib import Path

from PIL import Image, ImageDraw, ImageFont


ROOT = Path(__file__).resolve().parents[1]
DATA = ROOT / "data" / "databases" / "default"
DB_PATH = DATA / "database.sqlite"
TEAM_ROOT = DATA / "images_teams"

BLACK = (25, 27, 31)
WHITE = (236, 234, 224)
RED = (190, 27, 43)
BLUE = (29, 68, 137)
SKY = (82, 169, 214)
NAVY = (22, 39, 72)
YELLOW = (235, 193, 31)
GREEN = (24, 117, 73)
PURPLE = (91, 39, 128)
ORANGE = (224, 111, 25)
CLARET = (105, 28, 53)
PINK = (224, 112, 151)
GREY = (126, 132, 139)


@dataclass(frozen=True)
class KitDesign:
    primary: tuple[int, int, int]
    secondary: tuple[int, int, int]
    accent: tuple[int, int, int]
    pattern: str


@dataclass(frozen=True)
class Club:
    name: str
    short: str
    slug: str
    home: KitDesign
    away: KitDesign


@dataclass(frozen=True)
class League:
    db_id: int
    name: str
    asset_dir: str
    logo_url: str
    clubs: tuple[Club, ...]
    first_names: tuple[str, ...]
    last_names: tuple[str, ...]


def kit(primary, secondary, pattern="solid", accent=WHITE):
    return KitDesign(primary, secondary, accent, pattern)


def club(name, short, slug, home, away):
    return Club(name, short, slug, home, away)


PREMIER = (
    club("AFC Bournemouth", "BOU", "afcbournemouth", kit(RED, BLACK, "stripes", WHITE), kit(WHITE, RED, "sash", BLACK)),
    club("Arsenal", "ARS", "arsenal", kit(RED, WHITE, "sleeves", NAVY), kit(NAVY, SKY, "waves", RED)),
    club("Aston Villa", "AVL", "astonvilla", kit(CLARET, SKY, "sleeves", YELLOW), kit(BLACK, CLARET, "pinstripes", SKY)),
    club("Brentford", "BRE", "brentford", kit(WHITE, RED, "stripes", BLACK), kit(SKY, NAVY, "center", WHITE)),
    club("Brighton & Hove Albion", "BHA", "brighton", kit(BLUE, WHITE, "stripes", YELLOW), kit(YELLOW, NAVY, "shoulders", BLUE)),
    club("Chelsea", "CHE", "chelsea", kit(BLUE, WHITE, "waves", RED), kit(WHITE, BLUE, "center", RED)),
    club("Coventry City", "COV", "coventry", kit(SKY, NAVY, "pinstripes", WHITE), kit(BLACK, SKY, "sash", WHITE)),
    club("Crystal Palace", "CRY", "crystalpalace", kit(RED, BLUE, "stripes", WHITE), kit(WHITE, RED, "sash", BLUE)),
    club("Everton", "EVE", "everton", kit(BLUE, WHITE, "solid", YELLOW), kit(YELLOW, BLUE, "shoulders", BLACK)),
    club("Fulham", "FUL", "fulham", kit(WHITE, BLACK, "sleeves", RED), kit(RED, BLACK, "halves", WHITE)),
    club("Hull City", "HUL", "hullcity", kit(ORANGE, BLACK, "stripes", WHITE), kit(BLACK, ORANGE, "chevron", WHITE)),
    club("Ipswich Town", "IPS", "ipswich", kit(BLUE, WHITE, "sleeves", RED), kit(WHITE, BLUE, "sash", RED)),
    club("Leeds United", "LEE", "leeds", kit(WHITE, NAVY, "solid", YELLOW), kit(NAVY, YELLOW, "pinstripes", WHITE)),
    club("Liverpool", "LIV", "liverpool", kit(RED, WHITE, "solid", YELLOW), kit(WHITE, GREEN, "halves", RED)),
    club("Manchester City", "MCI", "manchestercity", kit(SKY, WHITE, "solid", NAVY), kit(BLACK, YELLOW, "sash", SKY)),
    club("Manchester United", "MUN", "manchesterunited", kit(RED, WHITE, "shoulders", BLACK), kit(WHITE, RED, "center", BLACK)),
    club("Newcastle United", "NEW", "newcastle", kit(WHITE, BLACK, "stripes", SKY), kit(GREEN, BLACK, "hoops", WHITE)),
    club("Nottingham Forest", "NFO", "nottinghamforest", kit(RED, WHITE, "solid", NAVY), kit(WHITE, RED, "pinstripes", BLUE)),
    club("Sunderland", "SUN", "sunderland", kit(WHITE, RED, "stripes", BLACK), kit(NAVY, RED, "center", WHITE)),
    club("Tottenham Hotspur", "TOT", "tottenham", kit(WHITE, NAVY, "sleeves", SKY), kit(NAVY, WHITE, "waves", YELLOW)),
)

SERIE_A = (
    club("Atalanta", "ATA", "atalanta", kit(BLUE, BLACK, "stripes", WHITE), kit(WHITE, BLUE, "sash", BLACK)),
    club("Bologna", "BOL", "bologna", kit(CLARET, NAVY, "stripes", WHITE), kit(WHITE, CLARET, "center", NAVY)),
    club("Cagliari", "CAG", "cagliari", kit(RED, NAVY, "halves", WHITE), kit(WHITE, RED, "cross", NAVY)),
    club("Como", "COM", "como", kit(BLUE, WHITE, "waves", YELLOW), kit(WHITE, BLUE, "sash", YELLOW)),
    club("Fiorentina", "FIO", "fiorentina", kit(PURPLE, WHITE, "solid", RED), kit(WHITE, PURPLE, "center", RED)),
    club("Frosinone", "FRO", "frosinone", kit(YELLOW, BLUE, "solid", WHITE), kit(WHITE, BLUE, "sash", YELLOW)),
    club("Genoa", "GEN", "genoa", kit(RED, NAVY, "halves", WHITE), kit(WHITE, RED, "quarters", NAVY)),
    club("Inter", "INT", "inter", kit(BLUE, BLACK, "stripes", YELLOW), kit(WHITE, SKY, "waves", BLUE)),
    club("Juventus", "JUV", "juventus", kit(WHITE, BLACK, "stripes", YELLOW), kit(SKY, NAVY, "center", WHITE)),
    club("Lazio", "LAZ", "lazio", kit(SKY, WHITE, "solid", NAVY), kit(NAVY, SKY, "sash", WHITE)),
    club("Lecce", "LEC", "lecce", kit(YELLOW, RED, "stripes", BLUE), kit(WHITE, RED, "sash", YELLOW)),
    club("Milan", "MIL", "milan", kit(RED, BLACK, "stripes", WHITE), kit(WHITE, RED, "center", BLACK)),
    club("Monza", "MON", "monza", kit(RED, WHITE, "center", BLACK), kit(WHITE, RED, "sash", BLACK)),
    club("Napoli", "NAP", "napoli", kit(SKY, WHITE, "solid", NAVY), kit(NAVY, SKY, "waves", WHITE)),
    club("Parma", "PAR", "parma", kit(WHITE, BLACK, "cross", YELLOW), kit(YELLOW, BLUE, "hoops", WHITE)),
    club("Roma", "ROM", "roma", kit(CLARET, ORANGE, "solid", WHITE), kit(WHITE, ORANGE, "sash", CLARET)),
    club("Sassuolo", "SAS", "sassuolo", kit(GREEN, BLACK, "stripes", WHITE), kit(WHITE, GREEN, "center", BLACK)),
    club("Torino", "TOR", "torino", kit(CLARET, WHITE, "solid", YELLOW), kit(WHITE, CLARET, "sash", NAVY)),
    club("Udinese", "UDI", "udinese", kit(WHITE, BLACK, "stripes", YELLOW), kit(ORANGE, BLACK, "pinstripes", WHITE)),
    club("Venezia", "VEN", "venezia", kit(BLACK, GREEN, "center", ORANGE), kit(WHITE, GREEN, "sash", ORANGE)),
)

LA_LIGA = (
    club("Athletic Club", "ATH", "athleticclub", kit(WHITE, RED, "stripes", BLACK), kit(BLACK, RED, "center", WHITE)),
    club("Atletico de Madrid", "ATM", "atleticomadrid", kit(WHITE, RED, "stripes", BLUE), kit(NAVY, RED, "sash", WHITE)),
    club("CA Osasuna", "OSA", "osasuna", kit(RED, NAVY, "solid", WHITE), kit(SKY, NAVY, "center", RED)),
    club("Celta de Vigo", "CEL", "celtavigo", kit(SKY, WHITE, "solid", RED), kit(CLARET, SKY, "sash", WHITE)),
    club("Deportivo Alaves", "ALA", "alaves", kit(BLUE, WHITE, "stripes", BLACK), kit(BLACK, BLUE, "chevron", WHITE)),
    club("Elche CF", "ELC", "elche", kit(WHITE, GREEN, "center", BLACK), kit(GREEN, WHITE, "pinstripes", RED)),
    club("FC Barcelona", "FCB", "fcbarcelona", kit(CLARET, BLUE, "stripes", YELLOW), kit(ORANGE, BLUE, "waves", CLARET)),
    club("Getafe CF", "GET", "getafe", kit(BLUE, WHITE, "solid", RED), kit(RED, BLUE, "sash", WHITE)),
    club("Levante UD", "LEV", "levante", kit(CLARET, BLUE, "stripes", WHITE), kit(WHITE, CLARET, "center", BLUE)),
    club("Malaga CF", "MAL", "malaga", kit(SKY, WHITE, "stripes", NAVY), kit(PURPLE, SKY, "pinstripes", WHITE)),
    club("Racing Club de Santander", "RAC", "racingsantander", kit(WHITE, GREEN, "center", BLACK), kit(GREEN, BLACK, "halves", WHITE)),
    club("Rayo Vallecano", "RAY", "rayovallecano", kit(WHITE, RED, "sash", BLACK), kit(BLACK, RED, "sash", WHITE)),
    club("RC Deportivo de La Coruna", "DEP", "deportivocoruna", kit(BLUE, WHITE, "stripes", RED), kit(ORANGE, BLUE, "center", WHITE)),
    club("RCD Espanyol", "ESP", "espanyol", kit(BLUE, WHITE, "stripes", BLACK), kit(PINK, NAVY, "sash", WHITE)),
    club("Real Betis", "BET", "realbetis", kit(WHITE, GREEN, "stripes", BLACK), kit(BLACK, GREEN, "pinstripes", WHITE)),
    club("Real Madrid", "RMA", "realmadrid", kit(WHITE, NAVY, "solid", YELLOW), kit(NAVY, WHITE, "waves", YELLOW)),
    club("Real Sociedad", "RSO", "realsociedad", kit(BLUE, WHITE, "stripes", BLACK), kit(ORANGE, NAVY, "sash", WHITE)),
    club("Sevilla FC", "SEV", "sevilla", kit(WHITE, RED, "solid", BLACK), kit(RED, WHITE, "pinstripes", BLACK)),
    club("Valencia CF", "VAL", "valencia", kit(WHITE, BLACK, "sleeves", ORANGE), kit(ORANGE, BLACK, "halves", WHITE)),
    club("Villarreal CF", "VIL", "villarreal", kit(YELLOW, NAVY, "solid", WHITE), kit(NAVY, YELLOW, "sash", WHITE)),
)

BUNDESLIGA = (
    club("FC Augsburg", "AUG", "augsburg", kit(WHITE, RED, "center", GREEN), kit(GREEN, BLACK, "halves", WHITE)),
    club("Union Berlin", "FCU", "unionberlin", kit(RED, WHITE, "solid", YELLOW), kit(WHITE, RED, "sash", YELLOW)),
    club("Werder Bremen", "SVW", "werderbremen", kit(GREEN, WHITE, "solid", ORANGE), kit(WHITE, GREEN, "sash", ORANGE)),
    club("Borussia Dortmund", "BVB", "borussiadortmund", kit(YELLOW, BLACK, "shoulders", WHITE), kit(BLACK, YELLOW, "pinstripes", WHITE)),
    club("SV Elversberg", "ELV", "elversberg", kit(BLUE, YELLOW, "center", WHITE), kit(WHITE, BLUE, "sash", YELLOW)),
    club("Eintracht Frankfurt", "SGE", "eintrachtfrankfurt", kit(RED, BLACK, "stripes", WHITE), kit(WHITE, BLACK, "center", RED)),
    club("SC Freiburg", "SCF", "freiburg", kit(RED, WHITE, "hoops", BLACK), kit(BLACK, RED, "sash", WHITE)),
    club("Hamburger SV", "HSV", "hamburgersv", kit(WHITE, BLUE, "solid", RED), kit(BLUE, BLACK, "halves", WHITE)),
    club("TSG Hoffenheim", "TSG", "hoffenheim", kit(BLUE, WHITE, "waves", SKY), kit(WHITE, BLUE, "pinstripes", SKY)),
    club("1. FC Koln", "KOE", "fckoln", kit(WHITE, RED, "solid", BLACK), kit(RED, WHITE, "center", BLACK)),
    club("RB Leipzig", "RBL", "rbleipzig", kit(WHITE, RED, "waves", BLUE), kit(NAVY, RED, "sash", WHITE)),
    club("Bayer Leverkusen", "B04", "bayerleverkusen", kit(RED, BLACK, "halves", WHITE), kit(WHITE, RED, "center", BLACK)),
    club("Mainz 05", "M05", "mainz05", kit(RED, WHITE, "quarters", BLACK), kit(BLACK, RED, "pinstripes", WHITE)),
    club("Borussia Monchengladbach", "BMG", "monchengladbach", kit(WHITE, GREEN, "center", BLACK), kit(GREEN, BLACK, "halves", WHITE)),
    club("Bayern Monaco", "FCB", "bayernmunich", kit(RED, WHITE, "waves", NAVY), kit(WHITE, RED, "center", NAVY)),
    club("SC Paderborn", "SCP", "paderborn", kit(BLUE, BLACK, "stripes", WHITE), kit(WHITE, BLUE, "sash", BLACK)),
    club("Schalke 04", "S04", "schalke04", kit(BLUE, WHITE, "solid", BLACK), kit(WHITE, BLUE, "center", BLACK)),
    club("VfB Stuttgart", "VFB", "stuttgart", kit(WHITE, RED, "hoops", BLACK), kit(RED, BLACK, "pinstripes", WHITE)),
)

LIGUE_1 = (
    club("Angers SCO", "ANG", "angers", kit(WHITE, BLACK, "stripes", YELLOW), kit(YELLOW, BLACK, "sash", WHITE)),
    club("AJ Auxerre", "AJA", "auxerre", kit(WHITE, BLUE, "center", RED), kit(BLUE, WHITE, "pinstripes", RED)),
    club("Stade Brestois 29", "BRE", "brest", kit(RED, WHITE, "solid", BLACK), kit(WHITE, RED, "sash", BLACK)),
    club("Le Havre AC", "HAC", "lehavre", kit(SKY, NAVY, "halves", WHITE), kit(WHITE, SKY, "center", NAVY)),
    club("Le Mans FC", "LMF", "lemans", kit(RED, YELLOW, "center", BLACK), kit(BLACK, RED, "sash", YELLOW)),
    club("RC Lens", "RCL", "lens", kit(YELLOW, RED, "stripes", BLACK), kit(BLACK, YELLOW, "center", RED)),
    club("FC Lorient", "LOR", "lorient", kit(ORANGE, BLACK, "solid", WHITE), kit(WHITE, ORANGE, "sash", BLACK)),
    club("LOSC Lille", "LIL", "lille", kit(RED, NAVY, "solid", WHITE), kit(WHITE, RED, "center", NAVY)),
    club("Olympique Lyonnais", "OL", "lyon", kit(WHITE, RED, "center", BLUE), kit(BLACK, RED, "sash", BLUE)),
    club("Olympique de Marseille", "OM", "marseille", kit(WHITE, SKY, "solid", NAVY), kit(NAVY, SKY, "waves", WHITE)),
    club("AS Monaco", "ASM", "monaco", kit(WHITE, RED, "diagonal", YELLOW), kit(NAVY, RED, "center", WHITE)),
    club("OGC Nice", "NIC", "nice", kit(RED, BLACK, "stripes", WHITE), kit(WHITE, RED, "sash", BLACK)),
    club("Paris FC", "PFC", "parisfc", kit(NAVY, SKY, "center", WHITE), kit(WHITE, NAVY, "sash", SKY)),
    club("Paris Saint-Germain", "PSG", "psg", kit(NAVY, RED, "center", WHITE), kit(WHITE, RED, "center", BLUE)),
    club("Stade Rennais", "REN", "rennes", kit(RED, BLACK, "halves", WHITE), kit(WHITE, RED, "sash", BLACK)),
    club("RC Strasbourg Alsace", "RCS", "strasbourg", kit(BLUE, WHITE, "solid", SKY), kit(WHITE, BLUE, "center", SKY)),
    club("Toulouse FC", "TFC", "toulouse", kit(PURPLE, WHITE, "solid", PINK), kit(WHITE, PURPLE, "sash", PINK)),
    club("ESTAC Troyes", "EST", "troyes", kit(BLUE, WHITE, "waves", SKY), kit(WHITE, BLUE, "pinstripes", SKY)),
)

EN_FIRST = ("Adam", "Ben", "Callum", "Daniel", "Ethan", "George", "Harry", "Jack", "James", "Lewis", "Oliver", "Ryan")
EN_LAST = ("Baker", "Bennett", "Clarke", "Cooper", "Davies", "Foster", "Gray", "Hughes", "Mason", "Parker", "Taylor", "Walker")
IT_FIRST = ("Alessio", "Andrea", "Carlo", "Daniele", "Davide", "Edoardo", "Federico", "Gabriele", "Lorenzo", "Marco", "Matteo", "Pietro")
IT_LAST = ("Barbieri", "Colombo", "Conti", "Costa", "Ferrara", "Galli", "Leone", "Lombardi", "Moretti", "Rinaldi", "Romano", "Santoro")
ES_FIRST = ("Adrian", "Alejandro", "Carlos", "Dani", "Diego", "Hector", "Javier", "Jorge", "Marcos", "Pablo", "Raul", "Sergio")
ES_LAST = ("Alonso", "Castro", "Delgado", "Garcia", "Iglesias", "Lopez", "Mendez", "Navarro", "Ortega", "Ramos", "Santos", "Vega")
DE_FIRST = ("Anton", "Felix", "Florian", "Jonas", "Julian", "Kai", "Leon", "Lukas", "Marcel", "Max", "Moritz", "Tobias")
DE_LAST = ("Bauer", "Becker", "Fischer", "Hartmann", "Keller", "Klein", "Krause", "Lehmann", "Richter", "Schmidt", "Vogel", "Wolf")
FR_FIRST = ("Adrien", "Alexis", "Antoine", "Baptiste", "Clement", "Enzo", "Hugo", "Jules", "Lucas", "Mathis", "Nathan", "Theo")
FR_LAST = ("Bernard", "Dubois", "Fournier", "Girard", "Laurent", "Lefevre", "Leroy", "Moreau", "Petit", "Roux", "Simon", "Thomas")

LEAGUES = (
    League(1, "Premier League", "premierleague", "images_competitions/premierleague.png", PREMIER, EN_FIRST, EN_LAST),
    League(2, "Bundesliga", "1bundesliga", "images_competitions/1bundesliga.png", BUNDESLIGA, DE_FIRST, DE_LAST),
    League(3, "Ligue 1", "ligue1", "images_competitions/ligue1.png", LIGUE_1, FR_FIRST, FR_LAST),
    League(4, "La Liga", "primeradivision", "images_competitions/primeradivision.png", LA_LIGA, ES_FIRST, ES_LAST),
    League(6, "Serie A", "serie_a", "images_competitions/serie_a.png", SERIE_A, IT_FIRST, IT_LAST),
)


def font(size: int):
    candidate = Path("C:/Windows/Fonts/arialbd.ttf")
    return ImageFont.truetype(str(candidate), size) if candidate.exists() else ImageFont.load_default()


def garment_mask():
    reference = Image.open(DATA / "template_kit.png").convert("RGB")
    mask = Image.new("L", reference.size, 0)
    src, dst = reference.load(), mask.load()
    for y in range(reference.height):
        for x in range(reference.width):
            r, g, b = src[x, y]
            if max(r, g, b) > 55:
                dst[x, y] = 255
    return mask


def crop_mask(base, boxes):
    result = Image.new("L", base.size, 0)
    for box in boxes:
        result.paste(base.crop(box), box[:2])
    return result


def shirt_pattern(design: KitDesign):
    image = Image.new("RGB", (1024, 1024), design.primary)
    draw = ImageDraw.Draw(image)
    p, s, a = design.primary, design.secondary, design.accent
    for offset in (0, 510):
        left, right = offset, offset + 505
        if design.pattern == "stripes":
            for x in range(left, right, 72):
                draw.rectangle((x + 36, 0, x + 71, 415), fill=s)
        elif design.pattern == "pinstripes":
            for x in range(left + 18, right, 38):
                draw.rectangle((x, 0, x + 6, 415), fill=s)
        elif design.pattern == "hoops":
            for y in range(35, 390, 76):
                draw.rectangle((left, y, right, y + 31), fill=s)
        elif design.pattern == "halves":
            draw.rectangle((left, 0, left + 252, 415), fill=s)
        elif design.pattern == "quarters":
            draw.rectangle((left, 0, left + 252, 207), fill=s)
            draw.rectangle((left + 252, 207, right, 415), fill=s)
        elif design.pattern == "center":
            draw.rectangle((left + 196, 0, left + 309, 415), fill=s)
            draw.rectangle((left + 239, 0, left + 266, 415), fill=a)
        elif design.pattern == "sash":
            draw.polygon(((left, 30), (left + 70, 0), (left + 420, 415), (left + 338, 415)), fill=s)
        elif design.pattern == "diagonal":
            draw.polygon(((left, 0), (left + 260, 0), (right, 245), (right, 415), (left + 420, 415), (left, 72)), fill=s)
        elif design.pattern == "cross":
            draw.rectangle((left + 205, 0, left + 300, 415), fill=s)
            draw.rectangle((left, 150, right, 225), fill=s)
        elif design.pattern == "chevron":
            draw.line(((left + 52, 85), (left + 252, 245), (left + 452, 85)), fill=s, width=55)
        elif design.pattern == "sleeves":
            draw.polygon(((left, 0), (left + 105, 0), (left + 105, 150), (left, 145)), fill=s)
            draw.polygon(((right - 105, 0), (right, 0), (right, 145), (right - 105, 150)), fill=s)
        elif design.pattern == "shoulders":
            draw.polygon(((left, 0), (right, 0), (right - 85, 105), (left + 85, 105)), fill=s)
        elif design.pattern == "waves":
            points = []
            for x in range(left, right + 1, 8):
                points.append((x, 170 + int(35 * __import__("math").sin((x - left) / 38))))
            draw.line(points, fill=s, width=58)
    draw.rectangle((0, 392, 1024, 414), fill=a)
    return image


def render_kit(design: KitDesign, mask, goalkeeper=False):
    if goalkeeper:
        design = KitDesign(design.accent, BLACK, design.primary, "chevron")
    shirt_mask = crop_mask(mask, ((0, 0, 1024, 415),))
    shorts_mask = crop_mask(mask, ((95, 415, 930, 580),))
    socks_mask = crop_mask(mask, ((0, 580, 565, 785),))
    canvas = Image.new("RGB", (1024, 1024), BLACK)
    canvas.paste(shirt_pattern(design), (0, 0), shirt_mask)
    canvas.paste(Image.new("RGB", canvas.size, design.secondary), (0, 0), shorts_mask)
    socks = Image.new("RGB", canvas.size, design.primary)
    ImageDraw.Draw(socks).rectangle((0, 600, 565, 632), fill=design.accent)
    canvas.paste(socks, (0, 0), socks_mask)
    return canvas.convert("RGBA")


def kit_signature(path: Path, mask: Image.Image) -> str:
    """Return seven normalized colour bins plus mean perceived luminance."""
    image = Image.open(path).convert("RGB")
    pixels = image.load()
    mask_pixels = mask.load()
    bins = [0.0] * 7  # dark neutral, light neutral, red, yellow, green, blue, purple
    luminance = 0.0
    samples = 0
    for y in range(2, 415, 4):
        for x in range(2, 1024, 4):
            if mask_pixels[x, y] == 0:
                continue
            r, g, b = pixels[x, y]
            rf, gf, bf = r / 255.0, g / 255.0, b / 255.0
            hue, saturation, value = colorsys.rgb_to_hsv(rf, gf, bf)
            luminance += 0.2126 * rf + 0.7152 * gf + 0.0722 * bf
            samples += 1
            if value < 0.30:
                bins[0] += 1.0
            elif saturation < 0.16:
                bins[1 if value >= 0.63 else 0] += 1.0
            else:
                degrees = hue * 360.0
                if degrees < 20.0 or degrees >= 340.0:
                    bins[2] += 1.0
                elif degrees < 75.0:
                    bins[3] += 1.0
                elif degrees < 165.0:
                    bins[4] += 1.0
                elif degrees < 265.0:
                    bins[5] += 1.0
                else:
                    bins[6] += 1.0
    if samples == 0:
        raise RuntimeError(f"No shirt pixels found in {path}")
    values = [value / samples for value in bins] + [luminance / samples]
    return ",".join(f"{value:.6f}" for value in values)


def badge(club_spec: Club):
    image = Image.new("RGBA", (128, 128), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    p, s = club_spec.home.primary, club_spec.home.secondary
    draw.ellipse((11, 8, 117, 116), fill=p + (255,), outline=WHITE + (255,), width=5)
    draw.polygon(((64, 15), (108, 42), (96, 99), (64, 118), (32, 99), (20, 42)), fill=s + (255,), outline=BLACK + (255,))
    draw.rectangle((17, 50, 111, 79), fill=p + (255,))
    f = font(27)
    box = draw.textbbox((0, 0), club_spec.short, font=f)
    draw.text(((128 - (box[2] - box[0])) / 2, 49), club_spec.short, font=f, fill=WHITE + (255,), stroke_width=2, stroke_fill=BLACK + (255,))
    return image


def make_ligue_logo():
    path = DATA / "images_competitions" / "ligue1.png"
    image = Image.new("RGBA", (128, 128), (0, 0, 0, 0))
    draw = ImageDraw.Draw(image)
    draw.rounded_rectangle((15, 15, 113, 113), radius=24, fill=NAVY + (255,), outline=(188, 233, 62, 255), width=7)
    draw.text((39, 30), "L1", font=font(37), fill=WHITE + (255,))
    image.save(path)


def build_assets():
    staging = ROOT / "tmp" / "real_2026_27_assets"
    if staging.exists():
        shutil.rmtree(staging)
    staging.mkdir(parents=True)
    mask = garment_mask()
    for league in LEAGUES:
        output = staging / league.asset_dir
        output.mkdir()
        for index, spec in enumerate(league.clubs):
            badge(spec).save(output / f"{spec.slug}_logo.png")
            for number, design in ((1, spec.home), (2, spec.away)):
                render_kit(design, mask).save(output / f"{spec.slug}_kit_{number:02d}.png")
                render_kit(design, mask, True).save(output / f"{spec.slug}_goalkeeper_kit_{number:02d}.png")
    obsolete = ("1bundesliga", "eredivisie", "ligue1", "premierleague", "primeradivision", "serie_a", "serie_a_fittizia")
    root_resolved = TEAM_ROOT.resolve()
    for directory in obsolete:
        target = (TEAM_ROOT / directory).resolve()
        if target.parent != root_resolved:
            raise RuntimeError(f"Unsafe asset target: {target}")
        if target.exists():
            shutil.rmtree(target)
    for league in LEAGUES:
        shutil.move(str(staging / league.asset_dir), str(TEAM_ROOT / league.asset_dir))
    shutil.rmtree(staging)
    make_ligue_logo()


def update_database():
    connection = sqlite3.connect(DB_PATH)
    connection.text_factory = lambda raw: raw.decode("cp1252")
    cursor = connection.cursor()
    template = cursor.execute(
        "SELECT nationalteam_id, role, age, base_stat, profile_xml, skincolor, hairstyle, haircolor, height, weight, formationorder, nationalteamformationorder "
        "FROM players WHERE team_id = 2 ORDER BY formationorder LIMIT 18"
    ).fetchall()
    formation, tactics = cursor.execute("SELECT formation_xml, tactics_xml FROM teams WHERE id = 2").fetchone()
    if len(template) != 18:
        raise RuntimeError("Could not load an 18-player template roster")
    rng = random.Random(20260928)
    signature_mask = garment_mask()
    cursor.execute("BEGIN")
    cursor.execute("DELETE FROM players")
    cursor.execute("DELETE FROM teams")
    cursor.execute("DELETE FROM sqlite_sequence WHERE name IN ('players', 'teams')")
    cursor.execute("DROP TABLE IF EXISTS team_kits")
    cursor.execute("CREATE TABLE team_kits(team_id INTEGER, kit_number INTEGER, outfield_signature TEXT, goalkeeper_signature TEXT, PRIMARY KEY(team_id, kit_number))")
    wanted_ids = {league.db_id for league in LEAGUES}
    cursor.execute("DELETE FROM leagues WHERE id NOT IN ({})".format(",".join("?" for _ in wanted_ids)), tuple(wanted_ids))
    for league in LEAGUES:
        cursor.execute("UPDATE leagues SET name = ?, logo_url = ? WHERE id = ?", (league.name, league.logo_url, league.db_id))
        if cursor.rowcount == 0:
            country_id = cursor.execute("SELECT id FROM countries ORDER BY id LIMIT 1").fetchone()[0]
            cursor.execute("INSERT INTO leagues(id, country_id, name, logo_url) VALUES (?, ?, ?, ?)", (league.db_id, country_id, league.name, league.logo_url))
        for team_index, spec in enumerate(league.clubs):
            prefix = f"images_teams/{league.asset_dir}/{spec.slug}"
            cursor.execute(
                "INSERT INTO teams(league_id,name,logo_url,kit_url,formation_xml,formation_factory_xml,tactics_xml,tactics_factory_xml,shortname,color1,color2) "
                "VALUES (?,?,?,?,?,?,?,?,?,?,?)",
                (league.db_id, spec.name, f"{prefix}_logo.png", prefix, formation, formation, tactics, tactics,
                 spec.short, ", ".join(map(str, spec.home.primary)), ", ".join(map(str, spec.home.secondary))),
            )
            team_id = cursor.lastrowid
            for kit_number in (1, 2):
                outfield_path = DATA / f"{prefix}_kit_{kit_number:02d}.png"
                goalkeeper_path = DATA / f"{prefix}_goalkeeper_kit_{kit_number:02d}.png"
                cursor.execute(
                    "INSERT INTO team_kits(team_id,kit_number,outfield_signature,goalkeeper_signature) VALUES (?,?,?,?)",
                    (team_id, kit_number, kit_signature(outfield_path, signature_mask), kit_signature(goalkeeper_path, signature_mask)),
                )
            for player_index, row in enumerate(template):
                _, role, age, stat, profile, skin, hairstyle, haircolor, height, weight, order, _ = row
                first = league.first_names[(team_index * 7 + player_index * 3) % len(league.first_names)]
                last = league.last_names[(team_index * 11 + player_index * 5) % len(league.last_names)]
                cursor.execute(
                    "INSERT INTO players(team_id,nationalteam_id,firstname,lastname,role,age,base_stat,profile_xml,skincolor,hairstyle,haircolor,height,weight,formationorder,nationalteamformationorder) "
                    "VALUES (?,-1,?,?,?,?,?,?,?,?,?,?,?,?,-1)",
                    (team_id, first, last, role, max(18, min(35, age + rng.randint(-3, 3))),
                     max(0.48, min(0.81, stat + rng.uniform(-0.04, 0.04))), profile, skin, hairstyle, haircolor, height, weight, order),
                )
    connection.commit()
    connection.close()


def sync_runtime():
    runtime_root = ROOT / "out" / "build-vs2026-x86"
    runtime = runtime_root / "databases" / "default"
    if runtime.parent.exists():
        runtime_teams = runtime / "images_teams"
        if runtime_teams.exists():
            shutil.rmtree(runtime_teams)
        shutil.copytree(TEAM_ROOT, runtime_teams)
        shutil.copy2(DB_PATH, runtime / "database.sqlite")
        shutil.copy2(DATA / "images_competitions" / "ligue1.png", runtime / "images_competitions" / "ligue1.png")


def refresh_signatures_only():
    connection = sqlite3.connect(DB_PATH)
    connection.text_factory = lambda raw: raw.decode("cp1252")
    cursor = connection.cursor()
    signature_mask = garment_mask()
    cursor.execute("DROP TABLE IF EXISTS team_kits")
    cursor.execute("CREATE TABLE team_kits(team_id INTEGER, kit_number INTEGER, outfield_signature TEXT, goalkeeper_signature TEXT, PRIMARY KEY(team_id, kit_number))")
    for team_id, prefix in cursor.execute("SELECT id, kit_url FROM teams").fetchall():
        for kit_number in (1, 2):
            outfield_path = DATA / f"{prefix}_kit_{kit_number:02d}.png"
            goalkeeper_path = DATA / f"{prefix}_goalkeeper_kit_{kit_number:02d}.png"
            cursor.execute(
                "INSERT INTO team_kits(team_id,kit_number,outfield_signature,goalkeeper_signature) VALUES (?,?,?,?)",
                (team_id, kit_number, kit_signature(outfield_path, signature_mask), kit_signature(goalkeeper_path, signature_mask)),
            )
    connection.commit()
    connection.close()
    runtime_db = ROOT / "out" / "build-vs2026-x86" / "databases" / "default" / "database.sqlite"
    if runtime_db.parent.exists():
        shutil.copy2(DB_PATH, runtime_db)
    print("Refreshed kit colour signatures.")


def main():
    if "--signatures-only" in sys.argv:
        refresh_signatures_only()
        return
    total = sum(len(league.clubs) for league in LEAGUES)
    if total != 96:
        raise RuntimeError(f"Expected 96 clubs, found {total}")
    build_assets()
    update_database()
    sync_runtime()
    print(f"Installed {total} real clubs with {total * 4} UV kit textures and {total} badges.")


if __name__ == "__main__":
    main()
