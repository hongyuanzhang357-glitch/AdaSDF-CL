#!/usr/bin/env python3
"""Release hygiene check for the AdaSDF-CL subtree.

Generated demo `.sdfbin` and `.svg` files must stay in build or temporary
directories.
"""

from __future__ import annotations

import argparse
from pathlib import Path


TEXT_SUFFIXES = {
    ".cmake",
    ".cpp",
    ".cff",
    ".h",
    ".hpp",
    ".json",
    ".md",
    ".py",
    ".txt",
    ".yaml",
    ".yml",
}


def is_text_file(path: Path) -> bool:
    return path.suffix.lower() in TEXT_SUFFIXES or path.name in {
        ".gitignore",
        ".gitattributes",
        "CMakeLists.txt",
    }


def should_skip(path: Path, root: Path) -> bool:
    rel_parts = {part.lower() for part in path.relative_to(root).parts}
    return bool({".git"} & rel_parts)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("root", nargs="?", default=".")
    args = parser.parse_args()

    root = Path(args.root).resolve()
    issues: list[str] = []

    forbidden_text = [
        "C:" + "/Users/",
        "C:" + "\\Users\\",
        "/m" + "nt/",
        "Poke" + "mon",
        "\u5b9d" + "\u53ef" + "\u68a6",
        "\u795e" + "\u5947" + "\u5b9d" + "\u8d1d",
    ]
    forbidden_parts = {
        "build",
        "cmakefiles",
        "dist",
        "cache",
        ".cache",
        "__pycache__",
        "build_results",
        "install",
        "install_tree",
        "package-build",
        "package_build",
        "package-test-build",
        "package_test_build",
        "downstream-build",
        "downstream_build",
        "_cpack_packages",
        "quarantine",
        "raw_stl",
    }
    forbidden_suffixes = {
        ".npz",
        ".sdfbin",
        ".zip",
        ".tgz",
        ".7z",
        ".rar",
        ".svg",
    }
    forbidden_archive_names = (
        ".tar",
        ".tar.gz",
        ".tar.xz",
        ".tar.bz2",
    )
    forbidden_generated_files = {
        "cmakecache.txt",
        "cmake_install.cmake",
        "cpackconfig.cmake",
        "cpacksourceconfig.cmake",
        "ctesttestfile.cmake",
        "install_manifest.txt",
    }
    remote_placeholder_terms = [
        "<" + "YOUR_GITHUB_REMOTE_URL" + ">",
        "<" + "URL" + ">",
        "github.com/" + "OWNER" + "/REPO",
        "github.com/" + "your-org" + "/your-repo",
        "git@github.com:" + "OWNER" + "/REPO.git",
    ]
    remote_placeholder_allowed = {
        "docs/github_publication_commands.md",
        "docs/adasdf_cl_github_publication_preflight_report.md",
    }

    for path in root.rglob("*"):
        if should_skip(path, root):
            continue
        rel = path.relative_to(root)
        rel_parts = {part.lower() for part in rel.parts}
        rel_text = str(rel).replace("\\", "/")

        if path.is_dir():
            if forbidden_parts & rel_parts:
                issues.append(f"forbidden directory name: {rel_text}")
            continue

        if not path.is_file():
            continue

        if path.stat().st_size > 100 * 1024 * 1024:
            issues.append(f"large file >100MB: {rel_text}")

        if forbidden_parts & rel_parts:
            issues.append(f"file inside forbidden directory: {rel_text}")

        suffix = path.suffix.lower()
        if suffix in forbidden_suffixes:
            issues.append(f"forbidden generated artifact: {rel_text}")

        lower_name = path.name.lower()
        if lower_name.endswith(forbidden_archive_names):
            issues.append(f"forbidden package archive: {rel_text}")

        if lower_name in forbidden_generated_files:
            issues.append(f"generated CMake file should not be committed: {rel_text}")

        if suffix == ".log":
            issues.append(f"log file should not be committed: {rel_text}")

        if lower_name in {"stdout", "stderr", "stdout.txt", "stderr.txt"}:
            issues.append(f"stdout/stderr artifact: {rel_text}")

        if is_text_file(path):
            try:
                text = path.read_text(encoding="utf-8", errors="ignore")
            except OSError as exc:
                issues.append(f"could not read text file {rel_text}: {exc}")
                continue
            for needle in forbidden_text:
                if needle in text:
                    issues.append(f"forbidden text in {rel_text}")
            if rel_text not in remote_placeholder_allowed:
                for needle in remote_placeholder_terms:
                    if needle in text:
                        issues.append(f"remote URL placeholder in public file: {rel_text}")

    readme = root / "README.md"
    if readme.exists():
        text = readme.read_text(encoding="utf-8", errors="ignore")
        if ("C:" + "/Users/") in text or ("C:" + "\\Users\\") in text:
            issues.append("README contains a local absolute path")

    docs = root / "docs"
    if docs.exists():
        for doc in docs.rglob("*.md"):
            text = doc.read_text(encoding="utf-8", errors="ignore")
            if ("C:" + "/Users/") in text or ("C:" + "\\Users\\") in text:
                issues.append(f"docs contain a local absolute path: {doc.relative_to(root)}")

    data = root / "data"
    if data.exists():
        readme_text = ""
        data_readme = data / "README.md"
        if data_readme.exists():
            readme_text = data_readme.read_text(encoding="utf-8", errors="ignore")
        if "project-generated" not in readme_text and "license" not in readme_text.lower():
            issues.append("data/ lacks project-generated or license documentation")

    if issues:
        print("Repo clean check: FAIL")
        for issue in issues:
            print(f"- {issue}")
        return 1

    print("Repo clean check: PASS")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
