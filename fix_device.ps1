################################################################################
# Quick Fix: Check Available Devices and Run with Alternative
# Run this script to find an available device and update your project
################################################################################

# Check what devices are available
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "Checking Installed Devices..." -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan

# List all available parts
vitis_hls -l > available_devices.txt

Write-Host "`nAvailable devices saved to: available_devices.txt" -ForegroundColor Green
Write-Host "Opening file..." -ForegroundColor Yellow
Start-Sleep -Seconds 2
notepad available_devices.txt

# Suggested alternatives
Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "SUGGESTED ALTERNATIVE DEVICES:" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "If you see these in available_devices.txt, you can use them:" -ForegroundColor Yellow
Write-Host ""
Write-Host "For Zynq-7000 (smaller, but works):" -ForegroundColor White
Write-Host "  xc7z020clg400-1" -ForegroundColor Green
Write-Host "  xc7z045ffg900-2" -ForegroundColor Green
Write-Host ""
Write-Host "For Zynq UltraScale+:" -ForegroundColor White
Write-Host "  xczu7ev-ffvc1156-2-e" -ForegroundColor Green
Write-Host "  xczu5ev-sfvc1156-2-e" -ForegroundColor Green
Write-Host ""

# Ask user which device to use
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "What would you like to do?" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""
Write-Host "Option 1: I found an xc7z device (Zynq-7000)" -ForegroundColor Yellow
Write-Host "Option 2: I found an xczu device (Zynq UltraScale+)" -ForegroundColor Yellow
Write-Host "Option 3: No devices found - I need to reinstall" -ForegroundColor Yellow
Write-Host ""
$choice = Read-Host "Enter your choice (1, 2, or 3)"

switch ($choice) {
    "1" {
        Write-Host "`nUsing Zynq-7000 device..." -ForegroundColor Green
        
        # Modify cnn_types.h for smaller device
        Write-Host "Reducing PE array size for Zynq-7000..." -ForegroundColor Yellow
        $typesFile = "include\cnn_types.h"
        $content = Get-Content $typesFile
        $content = $content -replace '#define M_SIZE 24', '#define M_SIZE 12  // Reduced for Zynq-7000'
        $content = $content -replace '#define N_SIZE 36', '#define N_SIZE 18  // Reduced for Zynq-7000'
        $content | Set-Content $typesFile
        
        # Modify build script
        Write-Host "Updating device in build_hls.tcl..." -ForegroundColor Yellow
        $tclFile = "scripts\build_hls.tcl"
        $tclContent = Get-Content $tclFile
        $tclContent = $tclContent -replace 'set part "xczu9eg-ffvb1156-2-e"', 'set part "xc7z020clg400-1"  # Changed to Zynq-7000'
        $tclContent | Set-Content $tclFile
        
        Write-Host "`n✅ Configuration updated for Zynq-7000 (12×18 = 216 PEs)" -ForegroundColor Green
        Write-Host "`nNow run: vitis_hls -f scripts\build_hls.tcl" -ForegroundColor Cyan
    }
    
    "2" {
        Write-Host "`nPlease enter the exact device part number from available_devices.txt" -ForegroundColor Yellow
        Write-Host "Example: xczu7ev-ffvc1156-2-e" -ForegroundColor Gray
        $device = Read-Host "Device part"
        
        # Modify build script
        Write-Host "Updating device in build_hls.tcl..." -ForegroundColor Yellow
        $tclFile = "scripts\build_hls.tcl"
        $tclContent = Get-Content $tclFile
        $tclContent = $tclContent -replace 'set part "xczu9eg-ffvb1156-2-e"', "set part `"$device`"  # Changed from ZCU102"
        $tclContent | Set-Content $tclFile
        
        Write-Host "`n✅ Configuration updated for device: $device" -ForegroundColor Green
        Write-Host "Keeping 24×36 PE array (should work on UltraScale+)" -ForegroundColor Green
        Write-Host "`nNow run: vitis_hls -f scripts\build_hls.tcl" -ForegroundColor Cyan
    }
    
    "3" {
        Write-Host "`nYou need to reinstall Vitis HLS with device support:" -ForegroundColor Red
        Write-Host ""
        Write-Host "1. Find your Vitis installer or download again" -ForegroundColor White
        Write-Host "2. Run as Administrator" -ForegroundColor White
        Write-Host "3. Select 'Add or Modify Installation'" -ForegroundColor White
        Write-Host "4. Check 'Zynq UltraScale+ MPSoC' under SoCs" -ForegroundColor White
        Write-Host "5. Complete installation" -ForegroundColor White
        Write-Host ""
        Write-Host "This will download ~5 GB and take 15-20 minutes" -ForegroundColor Yellow
    }
    
    default {
        Write-Host "`n❌ Invalid choice. Please run this script again." -ForegroundColor Red
    }
}

Write-Host "`n========================================" -ForegroundColor Cyan
Write-Host "Script Complete" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
