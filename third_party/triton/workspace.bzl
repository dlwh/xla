"""Provides the repository macro to import Triton."""

load("//third_party:repo.bzl", "tf_http_archive", "tf_mirror_urls")

def repo():
    """Imports Triton."""

    TRITON_COMMIT = "cl574102539"
    TRITON_SHA256 = "48c566a0e3f0c8e464371d0d4efd055eab8f73ff2d87fe4dc979675d990472f0"

    tf_http_archive(
        name = "triton",
        sha256 = TRITON_SHA256,
        strip_prefix = "triton-{commit}".format(commit = TRITON_COMMIT),
        urls = tf_mirror_urls("https://github.com/openxla/triton/archive/{commit}.tar.gz".format(commit = TRITON_COMMIT)),
        # For temporary changes which haven't landed upstream yet.
        patch_file = [
            "//third_party/triton:cl568176943.patch",
            "//third_party/triton:b304456327.patch",
        ],
    )
