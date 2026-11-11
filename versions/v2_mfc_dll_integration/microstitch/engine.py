from __future__ import annotations

from typing import Any, Dict, Optional
import numpy as np

from microstitch.mosaic import MosaicManager
from microstitch.mosaic_renderer import MosaicCanvasRenderer
from microstitch.registration import RegistrationEngine


class MicroStitchEngine:
    """Reusable stateful 2D microscope stitching engine.

    Encapsulates registration, coordinate accumulation, mosaic tile tracking,
    and canvas rendering into a clean interface suitable for DLL integration.
    """

    def __init__(
        self,
        min_response: float = 0.05,
        min_spatial: float = 0.35,
        max_shift: Optional[float] = 150.0,
    ) -> None:
        """Initialize stitching engine and underlying pipeline components.

        Args:
            min_response (float): Phase correlation peak threshold. Default: 0.05.
            min_spatial (float): Minimum spatial cross-correlation score. Default: 0.35.
            max_shift (Optional[float]): Maximum allowed displacement in pixels. Default: 150.0.
        """
        self.min_response = min_response
        self.min_spatial = min_spatial
        self.max_shift = max_shift

        self._engine = RegistrationEngine(
            min_response=min_response,
            min_spatial_score=min_spatial,
            max_shift=max_shift,
            use_laplacian=True,
            use_sift=True,
        )
        self._mosaic = MosaicManager()
        self._renderer = MosaicCanvasRenderer()
        self._prev_frame: Optional[np.ndarray] = None

    def process_frame(self, frame: np.ndarray) -> Dict[str, Any]:
        """Process a single camera frame through registration and mosaic building.

        Args:
            frame (np.ndarray): Input frame array (grayscale or BGR).

        Returns:
            Dict[str, Any]: Dictionary containing:
                - mosaic (Optional[np.ndarray]): Current rendered mosaic canvas.
                - registration_valid (bool): True if registration passed quality gates.
                - dx (float): Horizontal displacement in pixels.
                - dy (float): Vertical displacement in pixels.
                - response (float): Phase correlation response strength.
                - quality_score (float): Overall registration quality score [0.0, 1.0].
                - spatial_score (float): Spatial overlap cross-correlation score.
                - tile_count (int): Number of placed tiles in mosaic.
        """
        if not isinstance(frame, np.ndarray):
            raise TypeError("Frame must be a numpy.ndarray")
        if frame.size == 0:
            raise ValueError("Frame array cannot be empty")

        # First frame: placed as origin
        if self._prev_frame is None:
            self._prev_frame = frame.copy()
            self._mosaic.add_first_frame(frame)
            rendered_mosaic = self.get_mosaic()
            return {
                "mosaic": rendered_mosaic,
                "registration_valid": True,
                "dx": 0.0,
                "dy": 0.0,
                "response": 1.0,
                "quality_score": 1.0,
                "spatial_score": 1.0,
                "tile_count": self._mosaic.tile_count,
            }

        # Subsequent frames: register against previous frame
        ref_frame = self._prev_frame
        self._prev_frame = frame.copy()

        reg = self._engine.register(ref_frame, frame)

        if reg.valid:
            self._mosaic.add_frame(frame, reg)

        rendered_mosaic = self.get_mosaic()

        return {
            "mosaic": rendered_mosaic,
            "registration_valid": reg.valid,
            "dx": reg.dx,
            "dy": reg.dy,
            "response": reg.response,
            "quality_score": reg.quality_score,
            "spatial_score": reg.spatial_score,
            "tile_count": self._mosaic.tile_count,
        }

    def get_mosaic(self) -> Optional[np.ndarray]:
        """Render and return the current mosaic canvas.

        Returns:
            Optional[np.ndarray]: Rendered mosaic image canvas, or None if no tiles placed.
        """
        if self._mosaic.tile_count == 0:
            return None
        return self._renderer.render(self._mosaic, blend=True)

    def reset(self) -> None:
        """Reset mosaic state and clear previous frame reference."""
        self._mosaic.reset()
        self._prev_frame = None
