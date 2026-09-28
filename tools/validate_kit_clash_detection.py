#!/usr/bin/env python3
"""Validate automatic outfield and goalkeeper kit selection for every pairing."""

import itertools
import sqlite3
from pathlib import Path

DB = Path(__file__).resolve().parents[1] / "data" / "databases" / "default" / "database.sqlite"


def distance(left, right):
    histogram = sum(abs(left[index] - right[index]) for index in range(7)) * 0.5
    luminance = abs(left[7] - right[7])
    return histogram * 0.85 + luminance * 0.15


def choose(signatures, home_id, away_id):
    best = None
    for home_kit in (1, 2):
        for away_kit in (1, 2):
            home_out = signatures[(home_id, home_kit)][0]
            away_out = signatures[(away_id, away_kit)][0]
            outfield = distance(home_out, away_out)
            for home_goalkeeper_kit in (1, 2):
                for away_goalkeeper_kit in (1, 2):
                    home_gk = signatures[(home_id, home_goalkeeper_kit)][1]
                    away_gk = signatures[(away_id, away_goalkeeper_kit)][1]
                    opponent = min(distance(home_gk, away_out), distance(away_gk, home_out))
                    own_team = min(distance(home_gk, home_out), distance(away_gk, away_out))
                    goalkeepers = distance(home_gk, away_gk)
                    valid = outfield >= 0.25 and opponent >= 0.15 and own_team >= 0.15
                    score = outfield + opponent * 0.50 + own_team * 0.15 + goalkeepers * 0.10
                    score += 0.08 if home_kit == 1 else 0.0
                    score += 0.02 if away_kit == 1 else 0.0
                    if not valid:
                        score -= max(0.0, 0.25 - outfield) * 3.0
                        score -= max(0.0, 0.15 - opponent) * 4.0
                        score -= max(0.0, 0.15 - own_team) * 2.0
                    candidate = (valid, score, home_kit, away_kit, home_goalkeeper_kit,
                                 away_goalkeeper_kit, outfield, opponent, own_team, goalkeepers)
                    if best is None or candidate[:2] > best[:2]:
                        best = candidate
    return best


def main():
    connection = sqlite3.connect(DB)
    teams = dict(connection.execute("SELECT id, name FROM teams"))
    by_name = {name: team_id for team_id, name in teams.items()}
    signatures = {}
    for team_id, number, outfield, goalkeeper in connection.execute(
        "SELECT team_id, kit_number, outfield_signature, goalkeeper_signature FROM team_kits"
    ):
        signatures[(team_id, number)] = (
            tuple(map(float, outfield.split(","))), tuple(map(float, goalkeeper.split(",")))
        )
    connection.close()
    if len(signatures) != len(teams) * 2:
        raise RuntimeError("Incomplete kit signature table")

    fixed_distances = []
    selected_distances = []
    opponent_goalkeeper_distances = []
    own_goalkeeper_distances = []
    clashes = []
    selection_counts = {(1, 1): 0, (1, 2): 0, (2, 1): 0, (2, 2): 0}
    for home_id, away_id in itertools.permutations(teams, 2):
        fixed_distances.append(distance(signatures[(home_id, 1)][0], signatures[(away_id, 2)][0]))
        result = choose(signatures, home_id, away_id)
        (valid, _, home_kit, away_kit, _, _, outfield, opponent, own_team, _) = result
        selected_distances.append(outfield)
        opponent_goalkeeper_distances.append(opponent)
        own_goalkeeper_distances.append(own_team)
        selection_counts[(home_kit, away_kit)] += 1
        if not valid:
            clashes.append((teams[home_id], teams[away_id], result))

    print(f"pairings={len(selected_distances)}")
    print(f"fixed_average={sum(fixed_distances) / len(fixed_distances):.4f}")
    print(f"selected_average={sum(selected_distances) / len(selected_distances):.4f}")
    print(f"selected_below_0.25={sum(value < 0.25 for value in selected_distances)}")
    print(f"minimum_selected={min(selected_distances):.4f}")
    print(f"goalkeeper_opponent_below_0.15={sum(value < 0.15 for value in opponent_goalkeeper_distances)}")
    print(f"minimum_goalkeeper_opponent_safety={min(opponent_goalkeeper_distances):.4f}")
    print(f"goalkeeper_own_below_0.15={sum(value < 0.15 for value in own_goalkeeper_distances)}")
    print(f"minimum_goalkeeper_own_safety={min(own_goalkeeper_distances):.4f}")
    print(f"invalid_selections={len(clashes)}")
    print(f"selection_counts={selection_counts}")
    for home_name, away_name in (("Juventus", "Udinese"), ("Udinese", "Juventus")):
        result = choose(signatures, by_name[home_name], by_name[away_name])
        print(f"{home_name} vs {away_name}: home_kit={result[2]} away_kit={result[3]} "
              f"home_gk={result[4]} away_gk={result[5]} separation={result[6]:.4f} "
              f"opponent_gk={result[7]:.4f}")
    if clashes:
        raise RuntimeError(f"Found {len(clashes)} invalid automatic kit selections")


if __name__ == "__main__":
    main()
