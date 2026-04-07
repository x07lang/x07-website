import type {ReactNode} from 'react';
import {Redirect} from '@docusaurus/router';

export default function McpInstall(): ReactNode {
  return <Redirect to="/hardproof/install" />;
}
