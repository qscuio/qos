from orchestrator.stages.boot import STAGE as BOOT_STAGE
from orchestrator.stages.build_examples import STAGE as BUILD_EXAMPLES_STAGE
from orchestrator.stages.build_image import STAGE as BUILD_IMAGE_STAGE
from orchestrator.stages.build_kernel import STAGE as BUILD_KERNEL_STAGE
from orchestrator.stages.build_tools import STAGE as BUILD_TOOLS_STAGE
from orchestrator.stages.configure import STAGE as CONFIGURE_STAGE
from orchestrator.stages.fetch import STAGE as FETCH_STAGE
from orchestrator.stages.patch import STAGE as PATCH_STAGE
from orchestrator.stages.prepare import STAGE as PREPARE_STAGE


PHASE1_STAGES = [
    FETCH_STAGE,
    PREPARE_STAGE,
    PATCH_STAGE,
    CONFIGURE_STAGE,
    BUILD_KERNEL_STAGE,
    BUILD_TOOLS_STAGE,
    BUILD_EXAMPLES_STAGE,
    BUILD_IMAGE_STAGE,
    BOOT_STAGE,
]
