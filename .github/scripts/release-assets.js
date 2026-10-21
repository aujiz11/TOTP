const crypto = require('node:crypto');
const fs = require('node:fs');
const os = require('node:os');
const path = require('node:path');
const { execFileSync } = require('node:child_process');

const packages = ['TOTP-windows-x86.zip', 'TOTP-linux-x86.zip'];

function trustedRun(run, repository, sha) {
  return run.head_repository?.full_name === repository && run.head_sha === sha &&
    ['push', 'workflow_dispatch'].includes(run.event);
}

function verifyZip(filename, platform) {
  const files = execFileSync('cmake', ['-E', 'tar', 'tf', filename], { encoding: 'utf8' })
    .trim().split(/\r?\n/).sort();
  const expected = [
    'pawno/include/totp.inc', 'qawno/include/totp.inc',
    `plugins/TOTP.${platform === 'windows' ? 'dll' : 'so'}`,
    `components/TOTP.${platform === 'windows' ? 'dll' : 'so'}`,
    platform === 'windows' ? 'libcrypto-3.dll' : 'libcrypto.so.3'
  ].sort();
  if (JSON.stringify(files) !== JSON.stringify(expected)) {
    throw new Error(`Unexpected ZIP contents: ${path.basename(filename)}: ${files.join(', ')}`);
  }
}

module.exports = async function attach({ github, context, core }) {
  const repo = context.repo;
  const repository = `${repo.owner}/${repo.repo}`;
  const actions = github.rest.actions;
  const releasesApi = github.rest.repos;
  const completedRun = context.payload.workflow_run;
  if (context.eventName === 'workflow_run' &&
      (completedRun.conclusion !== 'success' ||
       !trustedRun(completedRun, repository, completedRun.head_sha))) {
    core.info('Ignoring an unsuccessful or untrusted package build.');
    return;
  }

  let releases;
  if (context.eventName === 'release') {
    const result = await releasesApi.getRelease({ ...repo, release_id: context.payload.release.id });
    releases = [result.data];
  } else if (context.eventName === 'workflow_dispatch') {
    const result = await releasesApi.getReleaseByTag({ ...repo, tag: context.payload.inputs.tag });
    releases = [result.data];
  } else {
    releases = await github.paginate(releasesApi.listReleases, { ...repo, per_page: 100 });
  }

  for (const release of releases) {
    if (release.draft || !release.published_at) continue;
    const { data: commit } = await releasesApi.getCommit({
      ...repo, ref: `refs/tags/${release.tag_name}`
    });
    const sha = commit.sha;
    if (context.eventName === 'workflow_run' && completedRun.head_sha !== sha) continue;

    const allRuns = await github.paginate(actions.listWorkflowRuns, {
      ...repo, workflow_id: 'packages.yml', head_sha: sha, per_page: 100
    });
    const runs = allRuns.filter(run => trustedRun(run, repository, sha))
      .sort((a, b) => b.run_number - a.run_number || b.run_attempt - a.run_attempt);
    let selected;
    let artifacts;
    for (const run of runs.filter(run => run.conclusion === 'success')) {
      const available = await github.paginate(actions.listWorkflowRunArtifacts, {
        ...repo, run_id: run.id, per_page: 100
      });
      const pair = packages.map(name => available.find(item => item.name === name && !item.expired));
      if (pair.every(Boolean)) {
        selected = run;
        artifacts = pair;
        break;
      }
    }
    if (!selected) {
      if (context.eventName === 'workflow_run') {
        throw new Error(`Successful package build has no complete ZIP pair for ${release.tag_name}.`);
      }
      if (runs.some(run => run.status !== 'completed')) {
        core.info(`Waiting for the package build for ${release.tag_name} (${sha}).`);
      } else {
        await actions.createWorkflowDispatch({ ...repo, workflow_id: 'packages.yml', ref: release.tag_name });
        core.info(`Requested an x86 package build for ${release.tag_name}; completion will attach both ZIPs.`);
      }
      continue;
    }

    const directory = fs.mkdtempSync(path.join(os.tmpdir(), 'totp-release-'));
    const downloads = [];
    try {
      for (const artifact of artifacts) {
        const redirect = await actions.downloadArtifact({
          ...repo, artifact_id: artifact.id, archive_format: 'zip', request: { redirect: 'manual' }
        });
        if (redirect.status !== 302 || !redirect.headers.location) {
          throw new Error(`No download URL for ${artifact.name}.`);
        }
        const response = await fetch(redirect.headers.location, { signal: AbortSignal.timeout(120000) });
        if (!response.ok) throw new Error(`Download failed for ${artifact.name}: ${response.status}`);
        const data = Buffer.from(await response.arrayBuffer());
        const digest = `sha256:${crypto.createHash('sha256').update(data).digest('hex')}`;
        if (artifact.digest !== digest) throw new Error(`Artifact digest mismatch: ${artifact.name}`);
        const filename = path.join(directory, artifact.name);
        fs.writeFileSync(filename, data);
        verifyZip(filename, artifact.name === packages[0] ? 'windows' : 'linux');
        downloads.push({ name: artifact.name, data, digest });
      }

      const { data: current } = await releasesApi.getRelease({ ...repo, release_id: release.id });
      const { data: currentCommit } = await releasesApi.getCommit({ ...repo, ref: `refs/tags/${current.tag_name}` });
      if (current.draft || !current.published_at || currentCommit.sha !== sha) {
        throw new Error(`Release/tag changed during download: ${release.tag_name}`);
      }
      for (const file of downloads) {
        const assets = await github.paginate(releasesApi.listReleaseAssets, {
          ...repo, release_id: current.id, per_page: 100
        });
        const existing = assets.find(asset => asset.name === file.name);
        if (existing?.digest === file.digest) {
          core.info(`${release.tag_name}: ${file.name} is already attached.`);
          continue;
        }
        if (existing) await releasesApi.deleteReleaseAsset({ ...repo, asset_id: existing.id });
        try {
          await releasesApi.uploadReleaseAsset({
            ...repo, release_id: current.id, name: file.name,
            label: `Actions run ${selected.id}; commit ${sha}`,
            data: file.data,
            headers: { 'content-type': 'application/zip', 'content-length': file.data.length }
          });
        } catch (error) {
          if (error.status !== 422) throw error;
          const latest = await github.paginate(releasesApi.listReleaseAssets, {
            ...repo, release_id: current.id, per_page: 100
          });
          if (!latest.some(asset => asset.name === file.name && asset.digest === file.digest)) throw error;
        }
        core.info(`Attached ${file.name} to ${release.tag_name} from run ${selected.id}.`);
      }
    } finally {
      for (const name of packages) fs.rmSync(path.join(directory, name), { force: true });
      fs.rmdirSync(directory);
    }
  }
};
