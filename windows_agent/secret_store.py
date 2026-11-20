"""Windows Credential Manager via keyring. No plaintext pairing key on disk."""
from __future__ import annotations

SERVICE = "MeoDeskCompanion"


def load(host: str) -> str:
    try:
        import keyring
        return keyring.get_password(SERVICE, host) or ""
    except Exception:
        return ""


def save(host: str, secret: str) -> bool:
    try:
        import keyring
        keyring.set_password(SERVICE, host, secret)
        return True
    except Exception:
        return False
