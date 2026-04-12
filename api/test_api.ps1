$api_url = "http://127.0.0.1:8000"

Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Log Processor API - Test Suite" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
Write-Host ""

# Test 1: Health Check
Write-Host "[1/3] Testing health endpoint..." -ForegroundColor Yellow
try {
    $response = Invoke-RestMethod -Uri "$api_url/health" -Method Get
    Write-Host "✓ Health check passed:" -ForegroundColor Green
    Write-Host ($response | ConvertTo-Json -Depth 1) -ForegroundColor Green
} catch {
    Write-Host "✗ Health check failed: $_" -ForegroundColor Red
    exit 1
}

Write-Host ""

# Test 2: Root endpoint
Write-Host "[2/3] Testing root endpoint..." -ForegroundColor Yellow
try {
    $response = Invoke-RestMethod -Uri "$api_url/" -Method Get
    Write-Host "✓ Root endpoint passed" -ForegroundColor Green
} catch {
    Write-Host "✗ Root endpoint failed: $_" -ForegroundColor Red
}

Write-Host ""

# Test 3: Process logs
Write-Host "[3/3] Testing log processing..." -ForegroundColor Yellow
try {
    $body = @{
        log_file = "data/sample.log"
    } | ConvertTo-Json
    
    $response = Invoke-RestMethod `
        -Uri "$api_url/process" `
        -Method Post `
        -ContentType "application/json" `
        -Body $body
    
    if ($response.success) {
        Write-Host "✓ Log processing succeeded!" -ForegroundColor Green
        Write-Host ""
        Write-Host "Statistics:" -ForegroundColor Cyan
        $response.data | ForEach-Object {
            $_ | Get-Member -MemberType NoteProperty | ForEach-Object {
                $name = $_.Name
                $value = $response.data.$name
                Write-Host "  $name`: $value"
            }
        }
    } else {
        Write-Host "✗ Processing failed: $($response.error)" -ForegroundColor Red
    }
} catch {
    Write-Host "✗ Log processing failed: $_" -ForegroundColor Red
}

Write-Host ""
Write-Host "========================================" -ForegroundColor Cyan
Write-Host "  Tests complete!" -ForegroundColor Cyan
Write-Host "========================================" -ForegroundColor Cyan
