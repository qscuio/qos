from __future__ import annotations

import argparse
import json
import sys
from pathlib import Path

from orchestrator.core.manifests import ManifestValidationError, load_manifests
from orchestrator.core.request import RequestValidationError, resolve_compat_request, resolve_native_request
from orchestrator.core.state import ensure_request_dirs, request_to_dict, write_json
from orchestrator.core.stages import execute_plan
from orchestrator.stages import PHASE1_STAGES


LINUX_LAB_ROOT = Path(__file__).resolve().parents[1]
MANIFEST_ROOT = LINUX_LAB_ROOT / "manifests"


def build_parser() -> argparse.ArgumentParser:
    parser = argparse.ArgumentParser(prog="linux-lab")
    subparsers = parser.add_subparsers(dest="command", required=True)

    for command in ("validate", "plan"):
        subparser = subparsers.add_parser(command)
        subparser.add_argument("--kernel", required=True)
        subparser.add_argument("--arch", required=True)
        subparser.add_argument("--image", required=True)
        subparser.add_argument("--profile", action="append", dest="profiles", required=True)
        subparser.add_argument("--mirror")

    return parser


def _load_all_manifests():
    return load_manifests(MANIFEST_ROOT)


def _native_request_from_args(args: argparse.Namespace):
    manifests = _load_all_manifests()
    request = resolve_native_request(
        manifests=manifests,
        kernel=args.kernel,
        arch=args.arch,
        image=args.image,
        profiles=args.profiles,
        mirror=args.mirror,
    )
    return manifests, request


def _plan(manifests, request) -> int:
    request_root = Path(request.artifact_root)
    ensure_request_dirs(request_root)
    write_json(request_root / "request.json", request_to_dict(request))
    execute_plan(request, manifests, PHASE1_STAGES)
    return 0


def run_compat(argv: list[str]) -> int:
    manifests = _load_all_manifests()
    request = resolve_compat_request(manifests=manifests, raw_args=argv)
    return _plan(manifests, request)


def main(argv: list[str] | None = None) -> int:
    parser = build_parser()
    args = parser.parse_args(argv)
    try:
        manifests, request = _native_request_from_args(args)
        if args.command == "validate":
            print(json.dumps(request_to_dict(request), indent=2, sort_keys=True))
            return 0
        return _plan(manifests, request)
    except (ManifestValidationError, RequestValidationError, RuntimeError) as exc:
        print(str(exc), file=sys.stderr)
        return 2
